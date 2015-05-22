CFLAGS= -O3

#CLIENT: standard c implementation
CLIENT_FILENAME=client/client.c
CLIENT_OUT=client/client.out

#CLIENT: c with kernel timestamping implementation
CLIENT_TIMESTAMP_FILENAME=client/timestamp_client.c
CLIENT_TIMESTAMP_OUT=client/timestamp_client.out

#CLIENT: kernel bypass with netmap implementation
CLIENT_NETMAP_FILENAME=client/netmap_client.c
CLIENT_NETMAP_OUT=client/netmap_client.out

#SERVER: standard c implementation
SERVER_FILENAME=server/server.c
SERVER_OUT=server/server.out

#SERVER: c with kernel timestamping implementation
SERVER_TIMESTAMP_FILENAME=server/timestamp_server.c
SERVER_TIMESTAMP_OUT=server/timestamp_server.out

#SERVER: kernel bypass with netmap implementation
SERVER_NETMAP_FILENAME=server/netmap_server.c
SERVER_NETMAP_OUT=server/netmap_server.out

HEADER=utils.h

default:
	@cat root_help.txt

client_c: $(CLIENT_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(CLIENT_FILENAME) -o $(CLIENT_OUT)

client_timestamp: $(CLIENT_TIMESTAMP_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(CLIENT_TIMESTAMP_FILENAME) -o $(CLIENT_TIMESTAMP_OUT)

client_netmap: $(CLIENT_NETMAP_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(CLIENT_NETMAP_FILENAME) -o $(CLIENT_NETMAP_OUT)

server_c: $(SERVER_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(SERVER_FILENAME) -o $(SERVER_OUT)

server_timestamp: $(SERVER_TIMESTAMP_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(SERVER_TIMESTAMP_FILENAME) -o $(SERVER_TIMESTAMP_OUT)

server_netmap: $(SERVER_NETMAP_FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(SERVER_NETMAP_FILENAME) -o $(SERVER_NETMAP_OUT)

clean:
	$(RM) $(CLIENT_OUT)
	$(RM) -r $(CLIENT_OUT).dSYM
	$(RM) $(CLIENT_TIMESTAMP_OUT)
	$(RM) -r $(CLIENT_TIMESTAMP_OUT).dSYM
	$(RM) $(CLIENT_NETMAP_OUT)
	$(RM) -r $(CLIENT_NETMAP_OUT).dSYM
	$(RM) $(SERVER_OUT)
	$(RM) -r $(SERVER_OUT).dSYM
	$(RM) $(SERVER_TIMESTAMP_OUT)
	$(RM) -r $(SERVER_TIMESTAMP_OUT).dSYM
	$(RM) $(SERVER_NETMAP_OUT)
	$(RM) -r $(SERVER_NETMAP_OUT).dSYM