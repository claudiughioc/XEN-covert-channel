#include "utils.h"

static int expired = 0;
static timer_t timer;
static char *buf;
static unsigned char *bits;

static void timer_handler(int signal)
{
	printf("Timer expired\n");
	expired = 1;
}

static void send_one(void)
{
	unsigned long long x = 1;
	printf("Sending one\n");

	start_timer(&timer, &expired);

	while (!expired) {
		x *= LARGE_PRIME;
	}

	stop_timer(&timer, &expired);
	printf("Finished work\n");
}

static void send_zero(void)
{

	printf("Sending zero\n");
	usleep(NSEC_TO_USEC(TIME_NSEC));
}


static void send(int val)
{
	switch (val) {
	case 1:
		send_one();
		break;
	case 0:
		send_zero();
		break;
	default:
		printf("Unable to send %d\n", val);
	}
}


int main(int argc, char **argv)
{
	int res = 0, count, bytes;
	struct timeval tim1, tim2;
	long long start, end;

	if (argc < 3) {
		printf("Usage: %s file_name nr_chars\n", argv[0]);
		return 1;
	}
	
	/* Initialize the sender */
	if ((res = init(&timer, timer_handler)))
		return res;

	bytes = atoi(argv[2]);
	if ((count = read_from_file(argv[1], &buf, bytes))< 0)
		return res;
	printf("Buffer to send is %s\n", buf);

	/* Transform bytes to bits */
	if ((res = bytes_to_bits(buf, &bits, bytes)))
		return res;

	gettimeofday(&tim1, NULL);
	send(1);
	send(1);
	send(0);
	gettimeofday(&tim2, NULL);
	start = tim1.tv_sec * 1000000ULL + tim1.tv_usec;
	end = tim2.tv_sec * 1000000ULL + tim2.tv_usec;
	printf("Sender started at %lld, finished at %lld\n", start, end);

	printf("All sent\n");

	free(buf);
	free(bits);
	return 0;
}
