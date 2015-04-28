#CC=gcc
CFLAGS= -g -Wall -Werror -O0
OUT=server.out
NETMAP_OUT=netmap_server.out
FILENAME=pi_server.c
NETMAP_FILENAME=pi_server_netmap.c
HEADER=../utils.h


$(OUT): $(FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(FILENAME) -o $(OUT)

run:
	./$(OUT)

clean:
	$(RM) $(OUT)
	$(RM) -r $(OUT).dSYM

netmap: $(NETMAP_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(NETMAP_FILENAME) -o $(NETMAP_OUT)