/* default, printf, etc */
#include<stdio.h>
/* socket stuff */
#include<sys/socket.h>
/* socket structs */
#include<netdb.h>
/* close() */
#include<unistd.h>

#include "common.h"

inline void close_socket(int sock) {
    printf("\nClosing socket...\n");
    close(sock);
}

void receive_packet(int *sock, void *buffer, size_t bufsize, int t_rcv[2], struct sockaddr_in *sender) {
    socklen_t senlen = sizeof(*sender);
    
    /* recv and log time */
    ssize_t bytes_recv = recvfrom(*sock, buffer, bufsize, 0, (struct sockaddr *) sender, &senlen);
    get_time(t_rcv);

    /* check for error */
    if(unlikely(bytes_recv < 0)) {
        close_socket(*sock);
        ERROR("ERROR receiving!");
        exit(EXIT_FAILURE);
    }
    
    return;
}

void send_packet(int *sock, void *data, size_t data_size, int t_rcv[2], struct sockaddr_in *receiver) {
    /* send and log time */
    ssize_t bytes_sent = sendto(*sock, data, data_size, 0, (struct sockaddr *) receiver, sizeof(*receiver));
    get_time(t_rcv);

    /* check for error */
    if(unlikely(bytes_sent < 0)) {
        close_socket(*sock);
        ERROR("ERROR sending!");
        exit(EXIT_FAILURE);
    }

    return;
}