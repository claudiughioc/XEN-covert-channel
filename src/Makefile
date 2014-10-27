CC = gcc
CFLAGS = -g -Wall -D_GNU_SOURCE
LDFLAGS = -lrt


all: sender receiver

sender: sender.c
	$(CC) $(CFLAGS) sender.c -o sender $(LDFLAGS)

receiver: receiver.c
	$(CC) $(CFLAGS) receiver.c -o receiver

clean:
	rm sender receiver
