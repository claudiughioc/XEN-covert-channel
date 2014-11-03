#include "utils.h"

void print_frame(struct worker worker)
{
	int i = 0;

	printf("Printing frame (%d, %d): ", worker.trans, worker.to_trans);
	for (i = 0; i < worker.to_trans; i++)
		printf("%d, ", worker.frame[i]);
	printf("\n");
	fflush(stdout);
}

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

static void send_one(struct backend *bck)
{
	unsigned long long x = 1;

	printf("Sending one\n");
	while (!bck->expired) {
		x *= LARGE_PRIME;
	}
}

static void send_zero(struct backend *bck)
{
	printf("Sending zero\n");
	usleep(NSEC_TO_USEC(BIT_TIME_NSEC - 20000000));
	while (!bck->expired);
}

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

	/* Set process afinity */
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
	char *bts = *bytes;

	if (!(bts = malloc(bits_no / 8 * sizeof(char)))) {
		printf("Error allocating bytes array\n");
		return -1;
	}

	for (i = 0; i < bits_no; i++) {
		idx = i % 8;
		if (idx == 0)
			bytes[count] = 0;

		bts[count] |= ((int)bits[i] & 1) << i;

		if (idx == 7) {
			printf("Obtained byte %d\n", (int) bts[count]);
			count++;
		}
	}

	*bytes = bts;

	return res;

}
