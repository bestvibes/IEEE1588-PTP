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

#include "../utils.h"

#define INTERFACE "eth0"

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
    d->self = d; //just to indicate this is a netmap configuration
    
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
    if (ioctl(d->fd, NIOCREGIF, &d->req)) {
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

int main() {
    struct nm_desc *d;
    struct pollfd fds;
    char *buf;
    struct nm_pkthdr h;
    int i;
    
    d = netmap_open();
    
    //signal handling to elegantly exit and close socket on ctrl+c
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    
    
    fds.fd = d->fd;
    fds.events = POLLIN;
    printf("ready to poll...\n");
    while(!stop) {
        printf("polling...\n");
        i = poll(&fds, 1, 5000);
        if (i > 0 && !(fds.revents & POLLERR)) {
            buf = netmap_nextpkt(d, &h);
            //37th and 38th bytes in packet are destination port
            if((buf[36] << 8 | buf[37]) == PORT) {
                printf("got something USEFUL... LEN:%d\n", h.len);
                //assume length will be shorter than to overflow to second byte (so a max length of 255)
                int len = buf[39] - 8;
                int num[2];
                int a, junk = 0;
                for(a = 0; a < len/4; a++) memcpy(&num[a], buf+42+(4*a), 4);
                for(a = 0; a < 2; a++) {
                    junk = ntohl(num[a]);
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
            i, fds.revents);
        }
    }
    netmap_close(d);
    return 0;
}