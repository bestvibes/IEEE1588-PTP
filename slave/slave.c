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
/* signal stuff */
#include<signal.h>

#include "../libs/common.h"
#include "../libs/stdnetwork.h"

void close_socket(int sock);

void get_time(int in[2]);

void receive_packet(int *sock, void *buffer, size_t bufsize, int t_rcv[2], struct sockaddr_in *sender);

void send_packet(int *sock, void *data, size_t data_size, int t_rcv[2], struct sockaddr_in *receiver);

void sig_handler(int signum) { }

/* calculate master-slave difference */
static void sync_packet(int *sock, struct sockaddr_in *master) {
    char useless_buffer[FIXED_BUFFER];
    int t2[2];
    receive_packet(sock, useless_buffer, FIXED_BUFFER, t2, NULL);
    send_packet(sock, t2, sizeof(t2), NULL, master);
}

/* calculate slave-master difference */
static void delay_packet(int *sock, struct sockaddr_in *master) {
    char useless_buffer[FIXED_BUFFER];
    int t3[2];
    receive_packet(sock, useless_buffer, FIXED_BUFFER, NULL, NULL);  /* wait for master to get ready */
    get_time(t3);
    send_packet(sock, t3, sizeof(t3), NULL, master);
}

/* IEEE 1588 PTP slave implementation */
static void sync_clock(int times, int *sock, struct sockaddr_in *master) {
    char useless_buffer[FIXED_BUFFER];
    int i; /* to prevent C99 error in for loop */
    
    printf("Running IEEE1588 PTP...");
    fflush(stdout);
    send_packet(sock, "ready", 5, NULL, master);

    /* run protocol num of times determined by master */
    for(i = 0; i < times; i++) {
        sync_packet(sock, master);
        delay_packet(sock, master);
        receive_packet(sock, useless_buffer, FIXED_BUFFER, NULL, NULL);
    }

    printf("Done!\n");
}

int main() {

    /* inits */
    int sock;
    struct sockaddr_in bind_addr;
    
    /* create socket file descriptor( AF_INET = ipv4 address family; SOCK_DGRAM = UDP; 0 = default protocol) */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if(unlikely(sock == -1)) {
        ERROR("ERROR creating socket!");
        exit(EXIT_FAILURE);
    } else {
        printf("Socket created!\n");
    }
    
    /* set details for socket to bind */
    memset(&bind_addr, '\0', sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;  /* bind to all interfaces */
    /* htons = host to network byte order, necessary for universal understanding by all machines */
    bind_addr.sin_port = htons(PORT);
    
    /* bind socket */
    if(unlikely(bind(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr)) < 0)) {
        close_socket(sock);
        ERROR("ERROR binding!\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Bound successfully!\n");
    }
    
    /* signal handling to elegantly exit and close socket on ctrl+c */
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    
    
    /* determine what to do */
    while(1) {
        printf("\nReady to receive requests on port %d...\n", PORT);
        struct sockaddr_in addr = {0};
        char buffer[FIXED_BUFFER] = {0};
        receive_packet(&sock, buffer, FIXED_BUFFER, NULL, &addr);
        if(strcmp(buffer, "sync") == 0) {
            send_packet(&sock, "ready", 5, NULL, &addr);
            int t = 0;
            receive_packet(&sock, &t, sizeof(t), NULL, NULL);
            sync_clock(t, &sock, &addr);
        }
        else {
            printf("Received invalid request...\n");
            send_packet(&sock, HELLO, sizeof(HELLO), NULL, &addr);
        }
    }
    return 0;
}
