#CC=gcc
CFLAGS= -g -Wall -Werror -O0
OUT=server.out
FILENAME=pi_server.c
HEADER=../utils.h


$(OUT): $(FILENAME) $(HEADER)
	$(CC) $(CFLAGS) $(FILENAME) -o $(OUT)

run:
	./$(OUT)

clean:
	$(RM) $(OUT)
	$(RM) -r $(OUT).dSYM