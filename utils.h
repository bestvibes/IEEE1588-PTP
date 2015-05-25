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

//USER-DEFINED SETTINGS
#define PORT 2468
//#define SLAVE_IP "192.168.137.2"
//#define SLAVE_IP "127.0.0.1"
#define SLAVE_IP "10.0.0.19"
#define NUM_OF_TIMES 1000  //num of times to run protocol

#define FIXED_BUFFER 16
#define POSITIVE 1
#define NEGATIVE -1

#define HELLO "Hello World!"

#ifndef likely
    #define likely(x)   __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#endif /* likely and unlikely */

#define TO_NSEC(t) (((long)t[0] * 1000000000L) + t[1])

#define ERROR(err)						        \
	do {										\
		char msg[128];							\
		sprintf(msg, "[FILE:%s|FUNCTION:%s|LINE:%d] %s",	\
		  __FILE__, __FUNCTION__, __LINE__, err);  \
		perror(msg);						 	\
		printf("Exitting...\n");				\
        } while (0)

inline void close_socket(int sock) {
    printf("\nClosing socket...\n");
    close(sock);
}

inline void get_time(int in[2]) {
    if(in != NULL) {
        //check for nanosecond resolution support
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

//for timestamp version
/*void recvpacket(int sock, struct sockaddr_in *cli_addr) {
	char data[256];
	struct msghdr msg;
	struct iovec entry;
	struct {
		struct cmsghdr cm;
		char control[512];
	} control;
	int bytes_recv;
    struct cmsghdr *cmsg;
    struct timespec ts;

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &entry;
	msg.msg_iovlen = 1;
	entry.iov_base = data;
	entry.iov_len = sizeof(data);
	msg.msg_name = (caddr_t)cli_addr;
	msg.msg_namelen = sizeof(*cli_addr);
	msg.msg_control = &control;
	msg.msg_controllen = sizeof(control);

	bytes_recv = recvmsg(sock, &msg, MSG_DONTWAIT);
	if (bytes_recv < 0) {
        close_socket(*sock);
        error("ERROR receiving!");
	} else {
		printpacket(&msg, res, data,
			    sock, recvmsg_flags,
			    siocgstamp, siocgstampns);
	}
}*/

void receive_packet(int *sock, void *buffer, size_t bufsize, int t_rcv[2], struct sockaddr_in *sender) {
    socklen_t senlen = sizeof(*sender);
    
    //recv and log time
    ssize_t bytes_recv = recvfrom(*sock, buffer, bufsize, 0, (struct sockaddr *) sender, &senlen);
    get_time(t_rcv);

    //check for error
    if(unlikely(bytes_recv < 0)) {
        close_socket(*sock);
        ERROR("ERROR receiving!");
        exit(EXIT_FAILURE);
    }
    
    return;
}

void send_packet(int *sock, void *data, size_t data_size, int t_rcv[2], struct sockaddr_in *receiver) {
    //send and log time
    ssize_t bytes_sent = sendto(*sock, data, data_size, 0, (struct sockaddr *) receiver, sizeof(*receiver));
    get_time(t_rcv);

    //check for error
    if(unlikely(bytes_sent < 0)) {
        close_socket(*sock);
        ERROR("ERROR sending!");
        exit(EXIT_FAILURE);
    }

    return;
}
