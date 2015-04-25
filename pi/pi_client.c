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
//gettimeofday()
#include<sys/time.h>
//time structs
#include<time.h>
//close()
#include <unistd.h>
//inet_addr struct
#include <arpa/inet.h>
//custom packet functions
#include "../utils.h"

#define FIXED_BUFFER 16
#define PORT 2468
#define BIND_PORT 2467 //if running on same machine
#define HELLO "Hello World!"
//#define ADDRESS "192.168.137.2"
#define ADDRESS "127.0.0.1"
#define NUM_OF_TIMES 1000

//call on error
void error(char *msg);

//convert timeval struct to manageable time representation
long parse_time(struct timeval *tv);

//function to receive a packet (sock fd, buff for output, time received, addr of client)
void receive_packet(int *sock, char buffer[FIXED_BUFFER], long *t_rcv, struct sockaddr_in *cli_addr);

//function to send packet (sock fd, client addr, data to be sent) returns time sent
long send_packet(int *sock, struct sockaddr_in *client, char data[]);

//calculate master-slave difference
void sync_packet(int *sock, struct sockaddr_in *server) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long t1 = parse_time(&tv);
	char t1_str[FIXED_BUFFER];
	sprintf(t1_str, "%ld", t1);
    send_packet(sock, server, t1_str);
}

//calculate slave-master difference
void delay_packet(int *sock, struct sockaddr_in *server) {
    long t4;
    char buff[FIXED_BUFFER] = {0};
	receive_packet(sock, buff, &t4, server);
	char t4_str[FIXED_BUFFER];  //not time-critical here, purpose of send just to send back to server
	sprintf(t4_str, "%ld", t4);
    send_packet(sock, server, t4_str);
}

//IEEE 1588 PTP Protocol implementation client-side
void sync_clock(int *sock, struct sockaddr_in *server) {
	//inits
	char useless_buffer[FIXED_BUFFER];
	
	//run protocol however number of times
    for(int i = 0; i < NUM_OF_TIMES; i++) {
        sync_packet(sock, server);
        delay_packet(sock, server);
		receive_packet(sock, useless_buffer, NULL, server);
    }
	printf("Done!\n");
}

int main() {

	//inits
    int sock;
    struct sockaddr_in bind_addr;
	struct sockaddr_in server_addr;
	
	//set details for socket to send data
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ADDRESS);  //send to server address
	//htons = host to network byte order, necessary for universal understanding by all machines
    server_addr.sin_port = htons(PORT);
    
	//create socket file descriptor( AF_INET = ipv4 address family; SOCK_DGRAM = UDP; 0 = default protocol)
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if(sock == -1){
        error("ERROR creating socket!");
    } else {
        printf("Socket created!\n");
    }
    
	//set details for socket to bind
    memset(&bind_addr, '\0', sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;  //bind to all interfaces
	//htons = host to network byte order, necessary for universal understanding by all machines
    bind_addr.sin_port = htons(BIND_PORT);
    
	//bind socket
    if(bind(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr)) < 0) {
        close(sock);
        error("ERROR binding!\n");
    } else {
        printf("Bound successfully!\n");
    }
    
	//sync time with server
    printf("Syncing time...\n");
	char buffer[FIXED_BUFFER] = {0};
	send_packet(&sock, &server_addr, "sync");
    receive_packet(&sock, buffer, NULL, &server_addr);
    if(strcmp(buffer, "ready") == 0) {
		//convert the defined int to a string to be sent
		char num_of_times[10];
		sprintf(num_of_times, "%d", NUM_OF_TIMES);
		send_packet(&sock, &server_addr, num_of_times);
		char buffer[FIXED_BUFFER] = {0};
        receive_packet(&sock, buffer, NULL, &server_addr);
		if(strcmp(buffer, "ready") == 0) {
			sync_clock(&sock, &server_addr);
		}
    }
    close(sock);
	return 0;
}