/* netmap!! */
#define NETMAP_WITH_LIBS
#include<net/netmap_user.h>

/* default, printf, etc */
#include<stdio.h>
/* uint32_t */
#include<stdint.h>
/* htons, htonl, etc */
#include<netinet/in.h>
/* signal stuff */
#include<signal.h>
/* pollfd */
#include<sys/poll.h>

#include "../libs/common.h"
#include "../libs/netmap_network.h"
#include "../libs/debug.h"

int stop = 0;

void sig_handler(int signum) { stop = 1; perror("Stopping"); };

void netmap_close(struct nm_desc *d);

struct nm_desc * netmap_open();

static char * netmap_nextpkt(struct nm_desc *d, struct nm_pkthdr *hdr);

struct pkt_ptp *init_ptp_packet(int ptp_mtype);

struct pkt_ptp *timestamp_ptp_packet(struct pkt_ptp *p);

int main() {
    struct nm_desc *d;
    struct netmap_ring *txring;
    struct pollfd pfd;
    struct netmap_if *nifp;
    char *buf;
    int i;
    uint8_t hwaddr[6];
    struct pkt_ptp *pkt;

    pkt = init_ptp_packet(SYNC);

    d = netmap_open();
    nifp = d->nifp;

    /* signal handling to elegantly exit and close socket on ctrl+c */
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




            buf = netmap_nextpkt(d, &h);
            /* 37th and 38th bytes in packet are destination port */
            if((buf[36] << 8 | buf[37]) == PORT) {
                printf("got something USEFUL... LEN:%d\n", h.len);
                /* assume length will be shorter than to overflow to second byte (so a max length of 255) */
                int len = buf[39] - 8;
                int num[2];
                int a, junk = 0;
                for(a = 0; a < len/4; a++) {
                    memcpy(&num[a], buf+42+(4*a), 4);
                    junk = ntohs(num[a]);
                }
                (void)junk;  /* just so compiler sees as used */
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
