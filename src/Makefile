CC = gcc
CFLAGS = -g -Wall -D_GNU_SOURCE
LDFLAGS = -lrt


all: sender receiver

sender: sender.c
	$(CC) $(CFLAGS) sender.c utils.c -o sender $(LDFLAGS)

receiver: receiver.c
	$(CC) $(CFLAGS) receiver.c utils.c -o receiver $(LDFLAGS)

clean:
	rm sender receiver
