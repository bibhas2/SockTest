CC=gcc
CFLAGS=-std=c99

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: test-client test-server

test-client: test-client.o
	gcc -o test-client test-client.o

test-server: test-server.o
	gcc -o test-server test-server.o
