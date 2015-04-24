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

#define FIXED_BUFFER 16

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
		//log time received
		gettimeofday(&tv, NULL);
		//assign data
		*cliAddr = temp_cliAddr;
	} else {
		//get packet (cliAddr and its length are optional params)
		bytes_recv = recvfrom(*sock, buffer, FIXED_BUFFER, 0, (struct sockaddr *) NULL, NULL);
		//log time received
		gettimeofday(&tv, NULL);
	}

	//parse and assign time
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