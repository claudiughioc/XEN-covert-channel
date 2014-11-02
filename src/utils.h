#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#define LARGE_PRIME		492876847
#define TIME_NSEC		50000000
#define NSEC_TO_USEC(x)		((x) / 1000ULL)

struct backend {
	timer_t timer;
	int expired;
	void (*timer_handler)(int);
};

int start_timer(struct backend *bck);
int stop_timer(struct backend *bck);
void send(int val, struct backend *bck);
unsigned long recv(struct backend *bck);
int init(struct backend *bck);

int read_from_file(char *file_name, char **buf, int size);
int bytes_to_bits(const char *buf, unsigned char **bits, int size);

#endif
