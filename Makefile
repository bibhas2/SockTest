CC=gcc
CFLAGS=-std=c99
OBJS=test-server.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test-server: $(OBJS)
	gcc -o test-server $(OBJS)
