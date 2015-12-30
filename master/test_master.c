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
/* gettimeofday() */
#include<sys/time.h>
/* time structs */
#include<time.h>
/* close() */
#include <unistd.h>
/* inet_addr struct */
#include <arpa/inet.h>

#define FIXED_BUFFER 16
#define PORT 2468
#define HELLO "Hello World!"
//#define ADDRESS "192.168.137.2"
//#define ADDRESS "127.0.0.1"
#define ADDRESS "10.0.0.19"

static inline void get_time(int in[2]) {
    if(in != NULL) {
        /* check for nanosecond resolution support */
        #ifndef CLOCK_REALTIME
            struct timeval tv = {0};
            gettimeofday(&tv, NULL);
            in[0] = (int) tv.tv_sec;
            in[1] = (int) tv.tv_usec * 1000;
        #else
            struct timespec ts = {0};
            clock_gettime(CLOCK_REALTIME, &ts);
            in[0] = (int) ts.tv_sec;
            in[1] = (int) ts.tv_nsec;
        #endif
    }
}

struct timeval send_packet(int *sock, struct sockaddr_in *client) {
    int data[2];
    get_time(data);
    printf("sending: %d %d\n", data[0], data[1]);
    struct timeval tv;
    sendto(*sock, &data, sizeof(data), 0, (struct sockaddr *) client, sizeof(*client));
    gettimeofday(&tv, NULL);
    return tv;
}

int main() {

    /* inits */
    int sock;
    struct sockaddr_in server_addr;
    
    /* set details for socket to send data */
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ADDRESS);  /* send to server address */
    /* htons = host to network byte order, necessary for universal understanding by all machines */
    server_addr.sin_port = htons(PORT);
    
    /* create socket file descriptor( AF_INET = ipv4 address family; SOCK_DGRAM = UDP; 0 = default protocol) */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if(sock == -1){
        printf("ERROR creating socket!");
    } else {
        printf("Socket created!\n");
    }

    send_packet(&sock, &server_addr);

    close(sock);
    return 0;
}