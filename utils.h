//default, printf, etc
#include<stdio.h>
//socket stuff
#include<sys/socket.h>
//socket structs
#include<netdb.h>
//gettimeofday()
#include<sys/time.h>
//time structs
#include<time.h>
//close()
#include<unistd.h>

#define FIXED_BUFFER 16

#define ERROR(_fmt, ...)						\
	do {										\
		char msg[64];							\
		sprintf(msg, "%s [%d] " _fmt,			\
		  __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
		perror(msg);						 	\
		printf("Exitting...\n");				\
        } while (0)

//call on error
void error(char *msg) {
    perror(msg);
    exit(1);
}

void close_socket(int sock) {
    printf("\nClosing socket...\n");
    close(sock);
}

//convert timeval struct to manageable time representation
long parse_time(struct timeval *tv) {
    return (((tv -> tv_sec % 3600) * 1000000)) + tv -> tv_usec;
}

//function to receive a packet (sock fd, buff for output, time received, addr of client)
void receive_packet(int *sock, char buffer[FIXED_BUFFER], long *t_rcv, struct sockaddr_in *cli_addr) {

    //inits
    struct timeval tv;
    ssize_t bytes_recv;

    if(cli_addr != NULL) {
        struct sockaddr_in temp_cli_addr = {0};
        socklen_t clilen = sizeof(temp_cli_addr);
        
        //get packet (cli_addr and its length are optional params)
        bytes_recv = recvfrom(*sock, buffer, FIXED_BUFFER, 0, (struct sockaddr *) &temp_cli_addr, &clilen);
        //log time received
        gettimeofday(&tv, NULL);
        //assign data
        *cli_addr = temp_cli_addr;
    } else {
        //get packet (cli_addr and its length are optional params)
        bytes_recv = recvfrom(*sock, buffer, FIXED_BUFFER, 0, (struct sockaddr *) NULL, NULL);
        //log time received
        gettimeofday(&tv, NULL);
    }

    //check for error
    if(bytes_recv < 0) {
        close_socket(*sock);
        error("ERROR receiving!");
    }

    //parse and assign time
    if(t_rcv != NULL) {
        long t = parse_time(&tv);
        *t_rcv = t;
    }
    
    //debug
    //printf("received: %s\n", buffer);
    
    return;
}

//function to send packet (sock fd, client addr, data to be sent) returns time sent
long send_packet(int *sock, struct sockaddr_in *client, char data[]) {
    struct timeval tv;
    sendto(*sock, data, strlen(data), 0, (struct sockaddr *) client, sizeof(*client));
    gettimeofday(&tv, NULL);
    return parse_time(&tv);
}