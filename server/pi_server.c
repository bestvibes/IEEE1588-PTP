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

#define FIXED_BUFFER 16
#define PORT 2468
#define HELLO "Hello World!"

//call on error
void error(char *msg) {
    perror(msg);
    exit(1);
}

//convert timeval struct to manageable time representation
long parseTime(struct timeval *tv) {
    return (((tv -> tv_sec % 3600) * 1000000)) + tv -> tv_usec;
}

//function to receive a packet (sock fd, buff for output, time received, addr of client)
void receivePacket(int *sock, char buffer[FIXED_BUFFER], long *t_rcv, struct sockaddr_in *cliAddr) {

	//inits
    struct timeval tv;
	ssize_t bytes_recv;

	if(cliAddr != NULL) {
		struct sockaddr_in temp_cliAddr = {0};
		socklen_t clilen = sizeof(temp_cliAddr);
		
		//get packet (cliAddr and its length are optional params)
		bytes_recv = recvfrom(*sock, buffer, FIXED_BUFFER, 0, (struct sockaddr *) &temp_cliAddr, &clilen);
		
		//assign data
		*cliAddr = temp_cliAddr;
	} else {
		//get packet (cliAddr and its length are optional params)
		bytes_recv = recvfrom(*sock, buffer, FIXED_BUFFER, 0, (struct sockaddr *) NULL, NULL);
	}

	//log time received
	gettimeofday(&tv, NULL);
	if(t_rcv != NULL) {
		long t = parseTime(&tv);
		*t_rcv = t;
	}
	
	//check for error
    if(bytes_recv < 0) {
        close(*sock);
        error("ERROR receiving!");
    }
	
	//debug
	//printf("received: %s\n", buffer);
	
    return;
}

//function to send packet (sock fd, client addr, data to be sent) returns time sent
long sendPacket(int *sock, struct sockaddr_in *client, char data[]) {
    struct timeval tv;
    sendto(*sock, data, strlen(data), 0, (struct sockaddr *) client, sizeof(*client));
    gettimeofday(&tv, NULL);
    return parseTime(&tv);
}

//calculate master-slave difference
long syncPacket(int *sock) {
    long t1, t2;
    char buff[FIXED_BUFFER] = {0};
	receivePacket(sock, buff, &t2, NULL);
	t1 = strtol(buff, (char **) NULL, 10);
    return t2 - t1;
}

//calculate slave-master difference
long delayPacket(int *sock, struct sockaddr_in *addr) {
    long t3, t4;
    t3 = sendPacket(sock, addr, "delayReq");
    char buff[FIXED_BUFFER] = {0};
	receivePacket(sock, buff, NULL, addr);
	t4 = strtol(buff, (char **) NULL, 10);
    return t4 - t3;
}

//IEEE 1588 PTP Protocol implementation
void syncClock(int *times, int *sock, struct sockaddr_in *client) {
	//inits
	int largest_offset = -999999999;
	int smallest_offset = 999999999;
	int sum_offset = 0;
	int smallest_delay = 999999999;
	int largest_delay = -999999999;
	int sum_delay = 0;
    int i = 0;
    sendPacket(sock, client, "ready");
	
	//run protocol num of times determined by slave
    for(i = 0; i < *times; i++) {
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
        close(sock);
        error("ERROR binding!\n");
    } else {
        printf("Bound successfully!\n");
    }
    
	//determine what to do
//    while(1) {
        printf("Ready to receive requests...\n");
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
//    }
    
    close(sock);
	return 0;
}