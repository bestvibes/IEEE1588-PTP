CFLAGS= -g -Wall -Werror -O0

#standard c implementation
FILENAME=client.c
OUT=client.out

#c with kernel timestamping implementation
TIMESTAMP_FILENAME=timestamp_client.c
TIMESTAMP_OUT=timestamp_client.out

#kernel bypass with netmap implementation
NETMAP_FILENAME=netmap_client.c
NETMAP_OUT=netmap_client.out

HEADER=../utils.h

default:
	@cat client_help.txt

c: $(FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(FILENAME) -o $(OUT)

timestamp: $(TIMESTAMP_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(TIMESTAMP_FILENAME) -o $(TIMESTAMP_OUT)

netmap: $(NETMAP_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(NETMAP_FILENAME) -o $(NETMAP_OUT)

clean:
	$(RM) $(OUT)
	$(RM) -r $(OUT).dSYM
	$(RM) $(TIMESTAMP_OUT)
	$(RM) -r $(TIMESTAMP_OUT).dSYM
	$(RM) $(NETMAP_OUT)
	$(RM) -r $(NETMAP_OUT).dSYM