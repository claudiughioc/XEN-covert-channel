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
#define BIT_TIME_NSEC		50000000
#define NSEC_TO_USEC(x)		((x) / 1000ULL)
#define SYNC_TIME		1		// SEC
#define MAX_SYNC_TIME		5000000		// uSEC
#define EXTRA_SYNC_TIME		1000000		// uSEC

#define FRAME_SIZE		32
#define FRAME_HEADER_SIZE	18
#define FRAME_TOTAL_SIZE	(FRAME_SIZE) + (FRAME_HEADER_SIZE)
#define FRAME_SEQ_SIZE		8
#define CRC_SIZE		8

struct backend {
	timer_t timer;
	timer_t sync_timer;
	int expired;
	int sync_expired;
	void (*timer_handler)(int);
	void (*sync_timer_handler)(int);
};

int start_timer(struct backend *bck);
int stop_timer(struct backend *bck);
void calibrate(struct backend *bck, unsigned long *zero_work,
		unsigned long *one_work, int is_sender);
void send(int val, struct backend *bck);
unsigned long recv(struct backend *bck);
int init(struct backend *bck);

int read_from_file(char *file_name, char **buf, int size);
int bytes_to_bits(const char *buf, unsigned char **bits, int size);

#endif
