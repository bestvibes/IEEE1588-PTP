CFLAGS= -O3

#master: standard c implementation
master_FILENAME=master/master.c
master_OUT=master/master.out

#master: c with kernel timestamping implementation
master_TIMESTAMP_FILENAME=master/timestamp_master.c
master_TIMESTAMP_OUT=master/timestamp_master.out

#master: kernel bypass with netmap implementation
master_NETMAP_FILENAME=master/netmap_master.c
master_NETMAP_OUT=master/netmap_master.out

#slave: standard c implementation
slave_FILENAME=slave/slave.c
slave_OUT=slave/slave.out

#slave: c with kernel timestamping implementation
slave_TIMESTAMP_FILENAME=slave/timestamp_slave.c
slave_TIMESTAMP_OUT=slave/timestamp_slave.out

#slave: kernel bypass with netmap implementation
slave_NETMAP_FILENAME=slave/netmap_slave.c
slave_NETMAP_OUT=slave/netmap_slave.out

HEADER=libs/

default:
	@cat root_help.txt

master_c: $(master_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(master_FILENAME) -o $(master_OUT)

master_timestamp: $(master_TIMESTAMP_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(master_TIMESTAMP_FILENAME) -o $(master_TIMESTAMP_OUT)

master_netmap: $(master_NETMAP_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(master_NETMAP_FILENAME) -o $(master_NETMAP_OUT)

slave_c: $(slave_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(slave_FILENAME) -o $(slave_OUT)

slave_timestamp: $(slave_TIMESTAMP_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(slave_TIMESTAMP_FILENAME) -o $(slave_TIMESTAMP_OUT)

slave_netmap: $(slave_NETMAP_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(slave_NETMAP_FILENAME) -o $(slave_NETMAP_OUT)

all: master_c master_timestamp master_netmap slave_c slave_timestamp slave_netmap

all_c: master_c slave_c

all_timestamp: master_timestamp slave_timestamp

all_netmap: master_netmap slave_netmap

clean:
	$(RM) $(master_OUT)
	$(RM) -r $(master_OUT).dSYM
	$(RM) $(master_TIMESTAMP_OUT)
	$(RM) -r $(master_TIMESTAMP_OUT).dSYM
	$(RM) $(master_NETMAP_OUT)
	$(RM) -r $(master_NETMAP_OUT).dSYM
	$(RM) $(slave_OUT)
	$(RM) -r $(slave_OUT).dSYM
	$(RM) $(slave_TIMESTAMP_OUT)
	$(RM) -r $(slave_TIMESTAMP_OUT).dSYM
	$(RM) $(slave_NETMAP_OUT)
	$(RM) -r $(slave_NETMAP_OUT).dSYM