/* netmap!! */
#define NETMAP_WITH_LIBS
#include<net/netmap_user.h>

/* default, printf, etc */
#include<stdio.h>
/* uint32_t */
#include<stdint.h>
/* ioctl stuff */
#include <sys/ioctl.h>
/* string stuff(memset, strcmp, strlen, etc) */
#include<string.h>
/* ether_header */
#include <netinet/if_ether.h>
/* ether_aton */
#include <netinet/ether.h>
/* htons, htonl, etc */
#include<netinet/in.h>
/* close() */
#include<unistd.h>

#include "common.h"
#include "debug.h"
#include "endian.h"

#define ETHERTYPE_PTP 0x88F7
#define DEST_MAC "01:1b:19:00:00:00" /*defined by IEEE-1588v2*/

struct ptp_header {
    uint8_t  ptp_ts:4,              /*transportSpecific*/
             ptp_type:4;            /*messageType*/
    uint8_t  ptp_v;                 /*versionPTP (4 bits in size)*/
    uint16_t ptp_ml;                /*messageLength*/
    uint8_t  ptp_dn;                /*domainNumber*/
    uint8_t  ptp_res;               /*reserved*/
    uint16_t ptp_flags;             /*flags*/
    uint64_t ptp_cf;                /*correctionField*/
    uint32_t ptp_res2;              /*reserved*/
    uint8_t  ptp_src[10];           /*sourcePortIdentity,8 byte mac,2 byte portId*/
    uint16_t ptp_sid;               /*sequenceId*/
    uint8_t  ptp_ctlf;              /*controlField*/
    uint8_t  ptp_lmi;               /*logMessageInterval*/
} __attribute__((__packed__));

struct pkt_ptp {
    struct ether_header eh;
    struct ptp_header ph;
    uint64_t s:48;
    uint32_t ns;
} __attribute__((__packed__));

enum ptp_msg_type
{
    SYNC = 0x0,
    DELAY_REQ = 0x1,
    FOLLOW_UP = 0x8,
    DELAY_RESP = 0x9,
};

void get_local_mac(uint8_t addr[6]) {
    int fd;
    struct ifreq req;

    fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    strcpy(req.ifr_name, NETMAP_INTERFACE);

    if(likely(ioctl(fd, SIOCGIFHWADDR, &req) == 0)) {
        memcpy(addr, req.ifr_hwaddr.sa_data, 6);
        close(fd);
    } else {
        ERROR(strcat("Unable to retrieve hwaddr for ", NETMAP_INTERFACE));
        close(fd);
        exit(EXIT_FAILURE);
    }
}

void netmap_close(struct nm_desc *d) {
    if (d == NULL)
        return;
    if (d->done_mmap && d->mem)
        munmap(d->mem, d->memsize);
    if (d->fd != -1)
        close(d->fd);
    memset(d, 0, sizeof(*d));
    free(d);
    printf("netmap closed...\n");
    return;
}

struct nm_desc * netmap_open() {
    struct nm_desc *d;
    uint32_t nr_flags = NR_REG_ALL_NIC;

    /* allocate memory and set up the descriptor */
    d = (struct nm_desc *)calloc(1, sizeof(*d));
    if (unlikely(d == NULL)) {
        ERROR("nm_desc alloc failure");
        exit(EXIT_FAILURE);
    }
    d->self = d; /* just to indicate this is a netmap configuration */

    printf("opening /dev/netmap...\n");
    d->fd = open("/dev/netmap", O_RDWR);
    if (unlikely(d->fd < 0)) {
        ERROR("cannot open /dev/netmap");
        netmap_close(d);
        exit(EXIT_FAILURE);
    }


    d->req.nr_version = NETMAP_API;
    d->req.nr_flags = nr_flags;
    strcpy(d->req.nr_name, NETMAP_INTERFACE);
    d->req.nr_name[strlen(d->req.nr_name)] = '\0'; /* null terminate the string */

    /* configure the network interface */
    printf("executing ioctl...\n");
    if (unlikely(ioctl(d->fd, NIOCREGIF, &d->req))) {
            ERROR("ioctl NIOCREGIF failed");
            netmap_close(d);
            exit(EXIT_FAILURE);
    }

    /* memory map netmap */
    printf("mmaping...\n");
    d->memsize = d->req.nr_memsize;
    d->mem = mmap(0, d->memsize, PROT_READ | PROT_WRITE, MAP_SHARED, d->fd, 0);
    if(unlikely(d->mem == MAP_FAILED)) {
        ERROR("mmap failed");
        netmap_close(d);
        exit(EXIT_FAILURE);
    }
    d->done_mmap = 1;

    /* set up rings locations */

    /* braces used for scope */
    {
        struct netmap_if *nifp = NETMAP_IF(d->mem, d->req.nr_offset);
        struct netmap_ring *r = NETMAP_RXRING(nifp, );

        *(struct netmap_if **)(uintptr_t)&d->nifp = nifp;
        *(struct netmap_ring **)(uintptr_t)&d->some_ring = r;
        *(void **)(uintptr_t)&d->buf_start = NETMAP_BUF(r, 0);
        *(void **)(uintptr_t)&d->buf_end = (char *) d->mem + d->memsize;
    }

    d->first_tx_ring = 0;
    d->first_rx_ring = 0;

    d->last_tx_ring = d->req.nr_tx_rings - 1;
    d->last_rx_ring = d->req.nr_rx_rings - 1;

    d->cur_tx_ring = d->first_tx_ring;
    d->cur_rx_ring = d->first_rx_ring;

    return d;
}

static char * netmap_nextpkt(struct nm_desc *d, struct nm_pkthdr *hdr) {

    int ri = d->cur_rx_ring;

    do {
        struct netmap_ring *ring = NETMAP_RXRING(d->nifp, ri);
        if(!nm_ring_empty(ring)) {
            u_int cur = ring->cur;
            u_int idx = ring->slot[cur].buf_idx;
            char *buf = (char *) NETMAP_BUF(ring, idx);

            hdr->ts = ring->ts;
            hdr->len = hdr->caplen = ring->slot[cur].len;
            ring->cur = nm_ring_next(ring, cur);
            ring->head = ring->cur;
            d->cur_rx_ring = ri;
            return buf;
        }
        ri++;
        if(ri > d->last_rx_ring) {
            ri = d->first_rx_ring;
        }
    } while(ri != d->cur_rx_ring);
    return NULL; /* nothing found :( */
}

/**
 * Construct a new PTP Ethernet packet.
 */
struct pkt_ptp *init_ptp_packet(int ptp_mtype) {
    int i, j = 0;
    struct pkt_ptp *pkt = malloc(sizeof(struct pkt_ptp));
    struct ether_header *eh = &pkt->eh;
    struct ptp_header *ph = &pkt->ph;
    uint8_t shost[6];

    get_local_mac(shost);

    eh->ether_type = htons(ETHERTYPE_PTP);              /*PTP over 802.3 ethertype*/
    memcpy(eh->ether_shost, shost, 6);                  /*source mac*/
    memcpy(eh->ether_dhost, ether_aton(DEST_MAC), 6);   /*destination mac*/

    ph->ptp_ts = 0;
    ph->ptp_type = ptp_mtype;
    ph->ptp_v = 2;                                      /*PTPv2 IEEE1588-2008*/
    ph->ptp_ml = htons(sizeof(struct pkt_ptp) - sizeof(struct ether_header));
    ph->ptp_dn = 0;
    ph->ptp_flags = 0;
    ph->ptp_cf = 0;

    for(i = 0; i < 10; i++) {
        if(i == 3)
            ph->ptp_src[i] = 0xff;
        else if(i == 4)
            ph->ptp_src[i] = 0xfe;
        else if(i == 8 || i == 9)
            ph->ptp_src[i] = 0x00;
        else {
            ph->ptp_src[i] = shost[j];
            j++;
        }
    }

    ph->ptp_sid = 0;  /*TODO, add sequence number on each protocol iteration*/
    ph->ptp_ctlf = 0;  /*deprecated in PTPv2*/
    ph->ptp_lmi = 0;   /*irrelevant*/

    pkt->s = 0;
    pkt -> ns = 0;
    return pkt;
}

/**
 * Timestamp a PTP Ethernet packet. Optimizations needed.
 */
struct pkt_ptp *timestamp_ptp_packet(struct pkt_ptp *p) {
    int t[2];
    get_time(t);
    union uint48_t n, n2;
    n.v = t[0];
    hton(n.c, 6, n2.c);
    dump_payload(n2.c, 6);
    p->s = n2.v;
    p->ns = htonl(t[1]);
    return p;
}