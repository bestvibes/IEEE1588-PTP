/* default, printf, etc */
#include<stdio.h>
/* socket stuff */
#include<sys/socket.h>
/* socket structs */
#include<netdb.h>
/* for LONG_MAX and LONG_MIN */
#include<limits.h>
/* strtol, other string conversion stuff */
#include<stdlib.h>
/* string stuff(memset, strcmp, strlen, etc) */
#include<string.h>
/* gettimeofday() */
#include<sys/time.h>
/* close() */
#include <unistd.h>
/* inet_addr struct */
#include <arpa/inet.h>

#include "../libs/common.h"
#include "../libs/stdnetwork.h"

void close_socket(int sock);

void get_time(int in[2]);

void receive_packet(int *sock, void *buffer, size_t bufsize, int t_rcv[2], struct sockaddr_in *sender);

void send_packet(int *sock, void *data, size_t data_size, int t_rcv[2], struct sockaddr_in *receiver);

/* calculate master-slave difference */
static long sync_packet(int *sock, struct sockaddr_in *slave) {
    int t1[2], t2[2];
    send_packet(sock, "sync_packet", 11, t1, slave);
    receive_packet(sock, t2, sizeof(t2), NULL, NULL);
    return (TO_NSEC(t2) - TO_NSEC(t1));
}

/* calculate slave-master difference */
static long delay_packet(int *sock, struct sockaddr_in *slave) {
    int t3[2], t4[2];
    send_packet(sock, "delay_packet", 12, NULL, slave);
    receive_packet(sock, t3, sizeof(t3), t4, NULL);
    return (TO_NSEC(t4) - TO_NSEC(t3));
}

/* IEEE 1588 PTP master implementation */
static void sync_clock(int *sock, struct sockaddr_in *slave) {
    long largest_offset = LONG_MIN;
    long smallest_offset = LONG_MAX;
    long sum_offset = 0;
    long smallest_delay = LONG_MAX;
    long largest_delay = LONG_MIN;
    long sum_delay = 0;
    int i; /* to prevent C99 error in for loop */
    char useless_buffer[FIXED_BUFFER];
    
    printf("Running IEEE1588 PTP %d times...\n", NUM_OF_TIMES);
    receive_packet(sock, useless_buffer, FIXED_BUFFER, NULL, NULL);

    /* run protocol however many number of times */
    for(i = 0; i < NUM_OF_TIMES; i++) {
        long ms_diff = sync_packet(sock, slave);
        long sm_diff = delay_packet(sock, slave);

        /* http://www.nist.gov/el/isd/ieee/upload/tutorial-basic.pdf  <- page 20 to derive formulas */
        long offset = (ms_diff - sm_diff)/2;
        long delay = (ms_diff + sm_diff)/2;

        /* calculate averages, min, max */
        sum_offset += offset;
        if (largest_offset < offset) {
            largest_offset = offset;
        }
        if (smallest_offset > offset) {
            smallest_offset = offset;
        }
        
        sum_delay += delay;
        if (largest_delay < delay) {
            largest_delay = delay;
        }
        if (smallest_delay > delay) {
            smallest_delay = delay;
        }

        send_packet(sock, "next", 4, NULL, slave);
    }

    /* print results */
    printf("Average Offset = %ldns\n", sum_offset/(NUM_OF_TIMES));
    printf("Average Delay = %ldns\n", sum_delay/(NUM_OF_TIMES));
    
    printf("Smallest Offset = %ldns\n", smallest_offset);
    printf("Smallest Delay = %ldns\n", smallest_delay);
    
    printf("Largest Offset = %ldns\n", largest_offset);
    printf("Largest Delay = %ldns\n", largest_delay);


    printf("Done!\n");
}

int main() {
    int sock;
    struct sockaddr_in slave_addr = {0};
    
    /* set details for socket to send data */
    // memset(&slave_addr, '\0', sizeof(slave_addr));
    slave_addr.sin_family = AF_INET;
    slave_addr.sin_addr.s_addr = inet_addr(SLAVE_IP);  /* send to slave address */
    /* htons = host to network byte order, necessary for universal understanding by all machines */
    slave_addr.sin_port = htons(PORT);
    
    /* create socket file descriptor(AF_INET = ipv4 address family; SOCK_DGRAM = UDP; 0 = default protocol) */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if(unlikely(sock == -1)) {
        ERROR("ERROR creating socket!");
        exit(EXIT_FAILURE);
    } else {
        printf("Socket created!\n");
    }
    
    /* sync time with slave */
    printf("Syncing time with %s:%d...\n\n", SLAVE_IP, PORT);
    char buffer[FIXED_BUFFER] = {0};
    send_packet(&sock, "sync", 4, NULL, &slave_addr);
    receive_packet(&sock, buffer, FIXED_BUFFER, NULL, &slave_addr);
    int num = NUM_OF_TIMES;
    send_packet(&sock, &num, sizeof(num), NULL, &slave_addr);
    sync_clock(&sock, &slave_addr);
    close_socket(sock);
    return 0;
}
