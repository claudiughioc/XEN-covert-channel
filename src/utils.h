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
#define BIT_TIME_NSEC		200000000
#define NSEC_TO_USEC(x)		((x) / 1000ULL)
#define MAX_SYNC_TIME		5000000		// uSEC
#define EXTRA_SYNC_TIME		1000000		// uSEC

#define FRAME_SIZE		32
#define FRAME_HEADER_SIZE	18
#define FRAME_TOTAL_SIZE	(FRAME_SIZE) + (FRAME_HEADER_SIZE)
#define FRAME_SEQ_SIZE		8
#define CRC_SIZE		8

typedef enum {
	SEND,
	SEND_INFO,
	RECV,
	RECV_INFO,
	RECV_ACK,
	WAIT
} work_id;

struct worker {
	char *buf;
	int buf_p;

	unsigned char *bits;		// buffer with the bits to send
	int bits_p;			// start of next frame, bits sent
	int size;

	unsigned char frame[FRAME_TOTAL_SIZE];
	int trans;			// bits sent inside a frame
	int to_trans;			// bits to send in a frame
	char frame_no;			// frame sequence number, starts at 1

	unsigned long threshold;	// decide whether it's one or zero
};

struct backend {
	timer_t timer;
	timer_t sync_timer;
	int expired;
	int sync_expired;
	void (*timer_handler)(int);
	void (*sync_timer_handler)(int);
};

void print_frame(struct worker worker);
int start_timer(struct backend *bck, long long start_diff);
int stop_timer(struct backend *bck);
void calibrate(struct backend *bck, unsigned long *zero_work,
		unsigned long *one_work, int is_sender);
void send(int val, struct backend *bck);
unsigned long recv(struct backend *bck);
int init(struct backend *bck);

int read_from_file(char *file_name, char **buf, int size);
int bytes_to_bits(const char *buf, unsigned char **bits, int size);
int bits_to_bytes(const unsigned char *bits, char **bytes, int bits_no);

#endif
