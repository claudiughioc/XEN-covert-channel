#include "utils.h"

int start_timer(struct backend *bck)
{
	struct itimerspec its;

	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = TIME_NSEC;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = TIME_NSEC;
	bck->expired = 0;

	timer_settime(bck->timer, 0, &its, NULL);

	return 0;
}

int stop_timer(struct backend *bck)
{
	struct itimerspec its;

	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	bck->expired = 0;

	return timer_settime(bck->timer, 0, &its, NULL);
}

static void send_one(struct backend *bck)
{
	unsigned long long x = 1;
	printf("Sending one\n");

	start_timer(bck);

	while (!bck->expired) {
		x *= LARGE_PRIME;
	}

	stop_timer(bck);
}

static void send_zero(void)
{

	printf("Sending zero\n");
	usleep(NSEC_TO_USEC(TIME_NSEC));
}

unsigned long recv(struct backend *bck)
{
	unsigned long long y = 1;
	unsigned long x = 0;

	start_timer(bck);

	while (!bck->expired) {
		y *= LARGE_PRIME;
		x++;
	}

	stop_timer(bck);

	return x;
}

void send(int val, struct backend *bck)
{
	switch (val) {
	case 1:
		send_one(bck);
		break;
	case 0:
		send_zero();
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

int init(struct backend *bck)
{
	int res = 0;
	struct sigevent sevp;
	struct sigaction sa;

	/* Set process afinity */
	if ((res = set_afin()))
		printf("Error setting cpu affinity\n");

	/* Set the sigalrm handler */
	memset(&sa, 0, sizeof (sa));
	sa.sa_handler = bck->timer_handler;
	if ((res = sigaction(SIGALRM, &sa, NULL))) {
		printf("Error setting the signal handler: %s\n", strerror(errno));
		return res;
	}


	/* Create the timer */
	sevp.sigev_notify = SIGEV_SIGNAL;
	sevp.sigev_signo = SIGALRM;
	if ((res = timer_create(CLOCK_MONOTONIC, &sevp, &bck->timer))) {
		printf("Error creating the timer %s\n", strerror(errno));
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

	return res;
}
