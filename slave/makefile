CFLAGS= -g -Wall -Werror -O0

#standard c implementation
FILENAME=slave.c
OUT=slave.out

#c with kernel timestamping implementation
TIMESTAMP_FILENAME=timestamp_slave.c
TIMESTAMP_OUT=timestamp_slave.out

#kernel bypass with netmap implementation
NETMAP_FILENAME=netmap_slave.c
NETMAP_OUT=netmap_slave.out


HEADER=../libs/

default:
	@cat slave_help.txt

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