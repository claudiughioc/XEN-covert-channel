#include "utils.h"

static const char crc_table[] = {
	0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31,
	0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
	0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, 0xe0, 0xe7, 0xee, 0xe9,
	0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
	0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1,
	0xb4, 0xb3, 0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
	0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0, 0xb9, 0xbe,
	0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
	0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16,
	0x03, 0x04, 0x0d, 0x0a, 0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
	0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80,
	0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
	0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8,
	0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
	0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10,
	0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
	0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f,
	0x6a, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
	0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7,
	0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
	0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef,
	0xfa, 0xfd, 0xf4, 0xf3
};

/* Compute CRC-8 */
unsigned char crc8(unsigned char *p, int len)
{
	unsigned char i;
	unsigned char crc = 0x0;

	while (len--) {
		i = (crc ^ *p++) & 0xFF;
		crc = (crc_table[i] ^ (crc << 8)) & 0xFF;
	}

	return crc & 0xFF;
}


/* Add bit to workers current frame */
void fill_frame(unsigned long work, struct worker *init)
{
	struct worker worker = *init;
	if (work < worker.threshold) {
		printf("R: Got a one with %lu\n", work);
		worker.frame[worker.trans++] = (int)1;
	} else {
		printf("R: Got a zero with %lu\n", work);
		worker.frame[worker.trans++] = (int)0;
	}

	*init = worker;
}

/* Print a frame */
void print_frame(struct worker worker)
{
	int i = 0;

	printf("Printing frame (%d, %d): ", worker.trans, worker.to_trans);
	for (i = 0; i < worker.to_trans; i++)
		printf("%d, ", worker.frame[i]);
	printf("\n");
	fflush(stdout);
}


/* Decide the zero and one threshold for the sender */
static void calibrate_sender(struct backend *bck, unsigned long *zero_work,
		unsigned long *one_work)
{
	printf("Calibrating the sender\n");
	send(1, bck);
	bck->expired = 0;
	*one_work = recv(bck);
	bck->expired = 0;
	send(0, bck);
	bck->expired = 0;
	*zero_work = recv(bck);
	bck->expired = 0;
}


/* Decide the zero and one threshold for the receiver */
static void calibrate_receiver(struct backend *bck, unsigned long *zero_work,
		unsigned long *one_work)
{
	printf("Calibrating the receiver\n");
	*one_work = recv(bck);
	bck->expired = 0;
	send(1, bck);
	bck->expired = 0;
	*zero_work = recv(bck);
	bck->expired = 0;
	send(0, bck);
	bck->expired = 0;
}

void calibrate(struct backend *bck, unsigned long *zero_work,
		unsigned long *one_work, int is_sender)
{
	bck->expired = 0;
	if (is_sender)
		calibrate_sender(bck, zero_work, one_work);
	else
		calibrate_receiver(bck, zero_work, one_work);
}


/* Send the bit 1 */
static void send_one(struct backend *bck)
{
	unsigned long long x = 1;

	printf("Sending one\n");
	while (!bck->expired) {
		x *= LARGE_PRIME;
	}
}


/* Send the bit 0 */
static void send_zero(struct backend *bck)
{
	printf("Sending zero\n");
	usleep(NSEC_TO_USEC(BIT_TIME_NSEC - 20000000));
	while (!bck->expired);
}


/* Receive a bit */
unsigned long recv(struct backend *bck)
{
	unsigned long long y = 1;
	unsigned long x = 0;

	while (!bck->expired) {
		y *= LARGE_PRIME;
		x++;
	}

	return x;
}


/* Send a bit */
void send(int val, struct backend *bck)
{
	switch (val) {
	case 1:
		send_one(bck);
		break;
	case 0:
		send_zero(bck);
		break;
	default:
		printf("Unable to send %d\n", val);
	}
}


/* Set processor affinity */
static int set_afin(void)
{
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(1, &mask);
	return sched_setaffinity(0, sizeof(mask), &mask);
}


int start_timer(struct backend *bck, long long start_diff)
{
	struct itimerspec its;

	printf("Starting the timer %p in %lld\n",
			&bck->timer, start_diff);
	its.it_value.tv_sec = start_diff / 1000000ULL;
	its.it_value.tv_nsec = (start_diff % 1000000ULL) * 1000ULL;
	printf("Synchronizing in %d seconds and %d nsecs\n",
			(int)its.it_value.tv_sec,
			(int)its.it_value.tv_nsec);
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = BIT_TIME_NSEC;
	bck->expired = 0;

	return timer_settime(bck->timer, 0, &its, NULL);
}

int stop_timer(struct backend *bck)
{
	struct itimerspec its;

	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	bck->expired = 0;

	return timer_settime(bck->timer, 0, &its, NULL);
}

int init(struct backend *bck)
{
	int res = 0;
	struct sigevent sevp;
	struct sigaction sa;
	struct timeval tim;
	long long now, sync_start;

	if (!bck->timer_handler) {
		printf("Provide the timer handler\n");
		return res;
	}

	bck->expired = 0;

	/* Set process affinity */
	if ((res = set_afin()))
		printf("Error setting cpu affinity\n");

	/* Set the sigalrm handler */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = bck->timer_handler;
	if ((res = sigaction(SIGALRM, &sa, NULL))) {
		printf("Error setting the signal handler: %s\n",
				strerror(errno));
		return res;
	}


	/* Create the bit timer */
	sevp.sigev_notify = SIGEV_SIGNAL;
	sevp.sigev_signo = SIGALRM;
	if ((res = timer_create(CLOCK_MONOTONIC, &sevp, &bck->timer))) {
		printf("Error creating the timer %s\n", strerror(errno));
		return res;
	}


	/* Start the sync timer */
	gettimeofday(&tim, NULL);
	now = tim.tv_sec * 1000000ULL + tim.tv_usec;
	sync_start = now - (now % MAX_SYNC_TIME) +
		MAX_SYNC_TIME + EXTRA_SYNC_TIME;
	if ((res = start_timer(bck, sync_start - now))) {
		printf("Unable to start sync timer %s\n", strerror(errno));
		return res;
	}

	return res;
}


/* Read from file into buffer, allocate it if needed
 * Don't forget to free buffer */
int read_from_file(char *file_name, char **buffer, int size)
{
	FILE *fp;
	int pos = 0, count;
	char *buf = *buffer;
	size_t left = size;

	if (!(fp = fopen(file_name, "r"))) {
		printf("Error opening file %s, %s\n",
				file_name, strerror(errno));
		return -1;
	}

	/* Allocate a bit more for frame padding */
	size = size - (size % FRAME_BYTES) + FRAME_BYTES;
	if (!(buf = malloc(size * sizeof(char)))) {
		printf("Unable to allocate buffer\n");
		return -1;
	}

	while (left && (count = fread(&buf[pos], sizeof(char),
					left < 64 ? left : 64, fp))) {
		pos += count;
		left -= count;
	}

	fclose(fp);
	*buffer = buf;
	return pos;
}


/* Write buffer to file */
int write_to_file(char *file_name, char *buffer, int size)
{
	FILE *fp;
	int pos = 0;

	if (!(fp = fopen(file_name, "w"))) {
		printf("Error opening file %s, %s\n",
				file_name, strerror(errno));
		return -1;
	}

	pos = fwrite(buffer, sizeof(char), size, fp);
	fclose(fp);

	return pos;
}


/* Create the bits array, represented in little endian
 * Don't forget to free bits */
int bytes_to_bits(const char *buf, unsigned char **bits, int size)
{
	int res = 0, count = 0, i, j;
	unsigned char *bts = *bits;

	if (!(bts = malloc(8 * size * sizeof(char)))) {
		printf("Error allocating bits array\n");
		return -1;
	}

	for (i = 0; i < size; i++)
		for (j = 0; j < 8; j++)
			bts[count++] = (unsigned char)((buf[i] >> j) & 1);
	*bits = bts;

	return res;
}


/* Create the bytes array, from bits represented in little endian
 * Don't forget to free bytes */
int bits_to_bytes(const unsigned char *bits, char **bytes, int bits_no)
{
	int res = 0, count = 0, i, idx;
	char *bts;

	if (!(bts = (char *)malloc(bits_no / 8 * sizeof(char)))) {
		printf("Error allocating bytes array\n");
		return -1;
	}

	for (i = 0; i < bits_no; i++) {
		idx = i % 8;
		if (idx == 0)
			bts[count] = 0;

		bts[count] |= ((int)bits[i] & 1) << idx;

		if (idx == 7)
			count++;
	}

	*bytes = bts;

	return res;

}
