#CC=gcc
CFLAGS= -g -Wall -Werror -O0
OUT=client.out
FILENAME=client.c
TIMESTAMP_FILENAME=client_timestamp.c
TIMESTAMP_OUT=client_timestamp.out
HEADER=../utils.h

$(OUT): $(FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(FILENAME) -o $(OUT)

run:
	./$(OUT)

clean:
	$(RM) $(OUT)
	$(RM) -r $(OUT).dSYM

timestamp:
	$(CC) $(CFLAGS) $(TIMESTAMP_FILENAME) -o $(TIMESTAMP_OUT)