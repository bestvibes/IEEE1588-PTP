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
long parseTime(struct timeval *tv);

//function to receive a packet (sock fd, buff for output, time received, addr of client)
void receivePacket(int *sock, char buffer[FIXED_BUFFER], long *t_rcv, struct sockaddr_in *cliAddr);

//function to send packet (sock fd, client addr, data to be sent) returns time sent
long sendPacket(int *sock, struct sockaddr_in *client, char data[]);

//calculate master-slave difference
void syncPacket(int *sock, struct sockaddr_in *server) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long t1 = parseTime(&tv);
	char t1Str[FIXED_BUFFER];
	sprintf(t1Str, "%ld", t1);
    sendPacket(sock, server, t1Str);
}

//calculate slave-master difference
void delayPacket(int *sock, struct sockaddr_in *server) {
    long t4;
    char buff[FIXED_BUFFER] = {0};
	receivePacket(sock, buff, &t4, server);
	char t4Str[FIXED_BUFFER];  //not time-critical here, purpose of send just to send back to server
	sprintf(t4Str, "%ld", t4);
    sendPacket(sock, server, t4Str);
}

//IEEE 1588 PTP Protocol implementation client-side
void syncClock(int *sock, struct sockaddr_in *server) {
	//inits
	char uselessBuffer[FIXED_BUFFER];
	
	//run protocol however number of times
    for(int i = 0; i < NUM_OF_TIMES; i++) {
        syncPacket(sock, server);
        delayPacket(sock, server);
		receivePacket(sock, uselessBuffer, NULL, server);
    }
	printf("Done!\n");
}

int main() {

	//inits
    int sock;
    struct sockaddr_in bindAddr;
	struct sockaddr_in serverAddr;
	
	//set details for socket to send data
    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ADDRESS);  //send to server address
	//htons = host to network byte order, necessary for universal understanding by all machines
    serverAddr.sin_port = htons(PORT);
    
	//create socket file descriptor( AF_INET = ipv4 address family; SOCK_DGRAM = UDP; 0 = default protocol)
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if(sock == -1){
        error("ERROR creating socket!");
    } else {
        printf("Socket created!\n");
    }
    
	//set details for socket to bind
    memset(&bindAddr, '\0', sizeof(bindAddr));
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_addr.s_addr = INADDR_ANY;  //bind to all interfaces
	//htons = host to network byte order, necessary for universal understanding by all machines
    bindAddr.sin_port = htons(BIND_PORT);
    
	//bind socket
    if(bind(sock, (struct sockaddr *) &bindAddr, sizeof(bindAddr)) < 0) {
        close(sock);
        error("ERROR binding!\n");
    } else {
        printf("Bound successfully!\n");
    }
    
	//sync time with server
    printf("Syncing time...\n");
	char buffer[FIXED_BUFFER] = {0};
	sendPacket(&sock, &serverAddr, "sync");
    receivePacket(&sock, buffer, NULL, &serverAddr);
    if(strcmp(buffer, "ready") == 0) {
		//convert the defined int to a string to be sent
		char num_of_times[10];
		sprintf(num_of_times, "%d", NUM_OF_TIMES);
		sendPacket(&sock, &serverAddr, num_of_times);
		char buffer[FIXED_BUFFER] = {0};
        receivePacket(&sock, buffer, NULL, &serverAddr);
		if(strcmp(buffer, "ready") == 0) {
			syncClock(&sock, &serverAddr);
		}
    }
    close(sock);
	return 0;
}