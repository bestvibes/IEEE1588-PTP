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

//custom packet functions
#include "../utils.h"

#define FIXED_BUFFER 16
#define PORT 2468
#define HELLO "Hello World!"

//call on error
void error(char *msg);

//close the socket
void close_socket(int sock);

//convert timeval struct to manageable time representation
long parseTime(struct timeval *tv);

//function to receive a packet (sock fd, buff for output, time received, addr of client)
void receivePacket(int *sock, char buffer[FIXED_BUFFER], long *t_rcv, struct sockaddr_in *cliAddr);

//function to send packet (sock fd, client addr, data to be sent) returns time sent
long sendPacket(int *sock, struct sockaddr_in *client, char data[]);

void sig_handler(int signum) { };

//calculate master-slave difference
long syncPacket(int *sock) {
    long t1, t2;
    char buff[FIXED_BUFFER] = {0};
	receivePacket(sock, buff, &t2, NULL);
	t1 = strtol(buff, (char **) NULL, 10);
    return t2 - t1;
}

//calculate slave-master difference
long delayPacket(int *sock, struct sockaddr_in *client) {
    long t3, t4;
    t3 = sendPacket(sock, client, "delayReq");
    char buff[FIXED_BUFFER] = {0};
	receivePacket(sock, buff, NULL, client);
	t4 = strtol(buff, (char **) NULL, 10);
    return t4 - t3;
}

//IEEE 1588 PTP Protocol implementation server-side
void syncClock(int *times, int *sock, struct sockaddr_in *client) {
	//inits
	int largest_offset = -999999999;
	int smallest_offset = 999999999;
	int sum_offset = 0;
	int smallest_delay = 999999999;
	int largest_delay = -999999999;
	int sum_delay = 0;
    sendPacket(sock, client, "ready");
	
	//run protocol num of times determined by slave
    for(int i = 0; i < *times; i++) {
        long msDiff = syncPacket(sock);
        long smDiff = delayPacket(sock, client);
		
		//debug
		//printf("msDiff = %ld ** ", msDiff);
		//printf("smDiff = %ld\n", smDiff);
		
		//http://www.nist.gov/el/isd/ieee/upload/tutorial-basic.pdf  <- page 20 to derive formulas
        int offset = (msDiff - smDiff)/2;
		long delay = (msDiff + smDiff)/2;
		
		//calculate averages, min, max
        sum_offset = sum_offset + offset;
		if (largest_offset < offset) {
			largest_offset = offset;
		}
		if (smallest_offset > offset) {
			smallest_offset = offset;
		}
        
        sum_delay = sum_delay + delay;
		if (largest_delay < delay) {
			largest_delay = delay;
		}
		if (smallest_delay > delay) {
			smallest_delay = delay;
		}
		
		sendPacket(sock, client, "next");
    }
	
	//print results
	printf("Average Offset = %d\n", sum_offset/(*times));
    printf("Average Delay = %d\n", sum_delay/(*times));
	
	printf("Smallest Offset = %d\n", smallest_offset);
    printf("Smallest Delay = %d\n", smallest_delay);
	
    printf("Largest Offset = %d\n", largest_offset);
    printf("Largest Delay = %d\n", largest_delay);
}

int main() {

	//inits
    int sock;
    struct sockaddr_in bindAddr;
    
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
    bindAddr.sin_port = htons(PORT);
    
	//bind socket
    if(bind(sock, (struct sockaddr *) &bindAddr, sizeof(bindAddr)) < 0) {
        close_socket(sock);
        error("ERROR binding!\n");
    } else {
        printf("Bound successfully!\n");
    }
    
	//signal handling to elegantly exit and close socket on ctrl+c
	struct sigaction sa;
	sa.sa_handler = sig_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	
	
	//determine what to do
    while(1) {
        printf("Ready to receive requests...\n\n");
        struct sockaddr_in addr;
		char buffer[FIXED_BUFFER] = {0};
        receivePacket(&sock, buffer, NULL, &addr);
        if(strcmp(buffer, "sync") == 0) {
			sendPacket(&sock, &addr, "ready");
			char buffer[FIXED_BUFFER] = {0};
            receivePacket(&sock, buffer, NULL, &addr);
            int times = (int) strtol(buffer, (char **) NULL, 10);
            syncClock(&times, &sock, &addr);
        }
		else {
			sendPacket(&sock, &addr, HELLO);
		}
    }
	return 0;
}