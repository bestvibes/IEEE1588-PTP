#CC=gcc
CFLAGS= -O3

FILENAME_CLIENT=client/client.c
OUT_CLIENT=client/client.out
FILENAME_CLIENT_TIMESTAMP=client/client_timestamp.c
OUT_CLIENT_TIMESTAMP=client/client_timestamp.out

FILENAME_SERVER=server/pi_server.c
OUT_SERVER=server/server.out
FILENAME_SERVER_NETMAP=server/server

HEADER=utils.h

default: $(OUT_CLIENT) $(OUT_SERVER)

$(OUT_CLIENT): $(FILENAME_CLIENT) $(HEADER)
	$(CC) $(CFLAGS) $(FILENAME_CLIENT) -o $(OUT_CLIENT)

$(OUT_SERVER): $(FILENAME_SERVER) $(HEADER)
	$(CC) $(CFLAGS) $(FILENAME_SERVER) -o $(OUT_SERVER)

clean:
	$(RM) $(OUT_CLIENT)
	$(RM) -r $(OUT_CLIENT).dSYM
	$(RM) $(OUT_SERVER)
	$(RM) -r $(OUT_SERVER).dSYM