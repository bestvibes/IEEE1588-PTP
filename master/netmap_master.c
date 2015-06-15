//netmap!!
#define NETMAP_WITH_LIBS
#include<net/netmap_user.h>

//pollfd
#include<sys/poll.h>
//default, printf, etc
#include<stdio.h>
//socket stuff
#include<sys/socket.h>
//socket structs
#include<netdb.h>
//strtol, other string conversion stuff
#include<stdlib.h>
//string stuff(memset, strcmp, strlen, etc)
#include<string.h>
//time structs
#include<time.h>
//signal stuff
#include<signal.h>
//uint32_t
#include<stdint.h>
//isprint()
#include <ctype.h>
//htons, etc
#include<netinet/in.h>
//ioctl stuff
#include <sys/ioctl.h>
//ifreq struct
#include <net/if.h>


#include "../utils.h"

#define INTERFACE "eth0"

#define PKT_SIZE 60
#define MASTER_IP 10.0.0.11
#define ETHERTYPE_PTP 0x88F7
#define DEST_MAC "01:1b:19:00:00:00"

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

struct pkt {
    struct ether_header eh;
    struct ip ip;
    struct udphdr udp;
    uint8_t body[16384];
} __attribute__((__packed__));

void close_socket(int sock);

int stop = 0;

void sig_handler(int signum) { stop = 1; perror("Stopping"); };

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

    //allocate memory and set up the descriptor
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
    strcpy(d->req.nr_name, INTERFACE);
    d->req.nr_name[strlen(d->req.nr_name)] = '\0'; //null terminate the string

    //configure the network interface
    printf("executing ioctl...\n");
    if (unlikely(ioctl(d->fd, NIOCREGIF, &d->req))) {
            ERROR("ioctl NIOCREGIF failed");
            netmap_close(d);
            exit(EXIT_FAILURE);
    }

    //memory map netmap
    printf("mmaping...\n");
    d->memsize = d->req.nr_memsize;
    d->mem = mmap(0, d->memsize, PROT_READ | PROT_WRITE, MAP_SHARED, d->fd, 0);
    if(unlikely(d->mem == MAP_FAILED)) {
        ERROR("mmap failed");
        netmap_close(d);
        exit(EXIT_FAILURE);
    }
    d->done_mmap = 1;

    //set up rings locations

    //braces used for scope
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
    return NULL; //nothing found :(
}


/* Check the payload of the packet for errors (use it for debug).
 * Prints payload in format recognizable by wireshark
 */
static void dump_payload(char *p, int len)
{
    char buf[128];
    int i, j, k = 0, i0;

    /* hexdump routine */
    for (i = 0; i < len; ) {
        memset(buf, sizeof(buf), ' ');
        sprintf(buf, "%04d: ", k);
        i0 = i;
        for (j=0; j < 16 && i < len; i++, j++)
            sprintf(buf+6+j*3, "%02x ", (uint8_t)(p[i]));
        i = i0;
        for (j=0; j < 16 && i < len; i++, j++)
            sprintf(buf+6+j + 48, "%c",
                isprint(p[i]) ? p[i] : '.');
        printf("%s\n", buf);
        k = k+10;
    }
}

void get_local_mac(uint8_t addr[6]) {
    int fd;
    struct ifreq req;

    fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    strcpy(req.ifr_name, INTERFACE);

    if(likely(ioctl(fd, SIOCGIFHWADDR, &req) == 0)) {
        memcpy(addr, req.ifr_hwaddr.sa_data, 6);
        close(fd);
    } else {
        ERROR(strcat("Unable to retrieve hwaddr for ", INTERFACE));
        close(fd);
        exit(EXIT_FAILURE);
    }
}

/**
 * Calculate checksum of IP header as per RFC791:
 * "The checksum field is the 16 bit one's complement of the one's complement sum of all 16 bit words in the header."
 */
uint16_t checksum(const void *header, uint16_t len, uint32_t sum) {
    const uint8_t *ptr = header;
    int i;

    for(i = 0; i < (len & ~1U); i+=2) {
        sum += (uint16_t) ntohs(*((uint16_t *) (ptr + i)));
        if(sum > 0xFFFF) {
            sum -= 0xFFFF;
        }
    }
    /*there's a leftover byte*/
    if(i < len) {
        sum += ptr[i] << 8;
        if(sum > 0xFFFF) {
            sum -= 0xFFFF;
        }
    }

    return sum;
}

struct pkt_ptp *init_ptp_packet(int ptp_type) {
    int i, j = 0;
    struct pkt_ptp *pkt;
    struct ether_header *eh = pkt->eh;
    struct ptp_header *ph = pkt->ph;
    uint8_t shost[6];

    get_local_mac(shost);

    int paylen = PKT_SIZE - sizeof(pkt->eh) - sizeof(pkt->ip);

    eh->ether_type = htons(ETHERTYPE_PTP);              /*PTP over 802.3 ethertype*/
    memcpy(eh->ether_shost, shost, 6);                  /*source mac*/
    memcpy(eh->ether_dhost, ether_aton(DEST_MAC), 6);   /*destination mac*/

    ph->ptp_ts = 0;
    ph->ptp_type = ptp_type;
    ph->ptp_v = 2;                                  /*PTPv2 IEEE1588-2008*/
    ph->ptp_ml = ;  /*TODO*/
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

    ph->ptp_sid = 0;  /*irrelevant at this point*/
    ph->ptp_ctlf = 0;  /*deprecated in PTPv2*/
    ph->ptp_lmi = 0;   /*irrelevant*/

    return pkt;
}

void init_packet(struct pkt *pkt, const char *payload) {
    struct ether_header *eh = pkt->eh;
    struct ip *ip = pkt->ip;
    struct udp *udp = pkt->udp;
    struct in_addr in_a = inet_aton(SLAVE_IP)

    int paylen = PKT_SIZE - sizeof(pkt->eh) - sizeof(pkt->ip);


    ip->ip_v = IPVERSION;                           /*ipv4*/
    ip->ip_hl = 5;                                  /*header length*/
    ip->ip_id = 0;                                  /*identification*/
    ip->ip_tos = IPTOS_LOWDELAY;                    /*type of service*/
    ip->ip_len = htons(PKT_SIZE - sizeof(pkt->eh)); /*total length*/
    ip->ip_off = htons(IP_DF);                      /*fragment offset field (don't fragment) */
    ip->ip_ttl = IPDEFTTL;                          /*time to live (64)*/
    ip->ip_p = IPPROTO_UDP;                         /*protocol (udp)*/
    ip->ip_dst.s_addr = inet_addr(SLAVE_IP);        /*dest address (inet_addr makes network byte order)*/
    ip->ip_src.s_addr = inet_addr(MASTER_IP);       /*src address (local address)*/
    ip->ip_sum = 0;                                 /*no need to calculate*/

    udp->uh_sport = htons(PORT);                    /*ports*/
    udp->uh_dport = htons(PORT);
    udp->uh_ulen = htons(paylen);                   /*length of payload including udp header*/
    udp->uh_sum = 0;                                /*no need to calculate*/

    eh->ether_type = htons(ETHERTYPE_PTP);          /*PTP over 802.3 ethertype*/
    eh->ether_shost =
    eh->ether_dhost =
}

int main() {
    struct nm_desc *d;
    struct netmap_ring *txring;
    struct pollfd pfd;
    struct netmap_if *nifp;
    char *buf;
    int i;
    uint8_t hwaddr[6];
    struct pkt_ptp pkt;

    init_ptp_packet(pkt);

    d = netmap_open();
    nifp = d->nifp;

    //signal handling to elegantly exit and close socket on ctrl+c
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);


    pfd.fd = d->fd;
    pfd.events = POLLOUT;
    printf("ready to send...\n");
    while(!stop) {
        printf("sending...\n");
        if (poll(&pfd, 1, 3000) <= 0 || pfd.revents & POLLERR) {
            ERROR("Poll Error: No room available in queue!");
            break;
        }

        for(i = d->first_tx_ring; i <= d->last_tx_ring; i++) {
            txring = NETMAP_TXRING(nifp, i);
            if(nm_ring_empty(txring))
                continue;

        }




            buf = netmap_nextpkt(d, &h);
            //37th and 38th bytes in packet are destination port
            if((buf[36] << 8 | buf[37]) == PORT) {
                printf("got something USEFUL... LEN:%d\n", h.len);
                //assume length will be shorter than to overflow to second byte (so a max length of 255)
                int len = buf[39] - 8;
                int num[2];
                int a, junk = 0;
                for(a = 0; a < len/4; a++) {
                    memcpy(&num[a], buf+42+(4*a), 4);
                    junk = ntohs(num[a]);
                }
                (void)junk;  //just so compiler sees as used
                printf("LELELEL: %d %d\n", num[0], num[1]);
                dump_payload(buf, h.len);
            } else {
                printf("got something USELESS... PORT:%d, LEN:%d\n", (buf[36] << 8 | buf[37]), h.len);
                if(h.len != 215)
                    dump_payload(buf, h.len);
            }
        } else {
            printf("waiting for initial packets, poll returns %d %d\n",
            i, pfd.revents);
        }
    }
    netmap_close(d);
    return 0;
}
