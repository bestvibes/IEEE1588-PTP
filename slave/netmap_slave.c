/* netmap!! */
#define NETMAP_WITH_LIBS
#include<net/netmap_user.h>

/* pollfd */
#include<sys/poll.h>
/* default, printf, etc */
#include<stdio.h>
/* socket stuff */
#include<sys/socket.h>
/* socket structs */
#include<netdb.h>
/* strtol, other string conversion stuff */
#include<stdlib.h>
/* string stuff(memset, strcmp, strlen, etc) */
#include<string.h>
/* time structs */
#include<time.h>
/* signal stuff */
#include<signal.h>
/* uint32_t */
#include<stdint.h>
/* isprint() */
#include <ctype.h>
/* htons, etc */
#include<netinet/in.h>

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
    struct pollfd pfd;
    char *buf;
    struct nm_pkthdr h;
    int i;
    
    d = netmap_open();
    
    /* signal handling to elegantly exit and close socket on ctrl+c */
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    
    
    pfd.fd = d->fd;
    pfd.events = POLLIN;
    printf("ready to poll...\n");
    while(!stop) {
        printf("polling...\n");
        i = poll(&pfd, 1, 5000);
        if (i > 0 && !(pfd.revents & POLLERR)) {
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