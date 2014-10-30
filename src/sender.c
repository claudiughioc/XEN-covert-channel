#include "utils.h"

static int expired = 0;
static timer_t timer;

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

int main(void)
{
	int res = 0;
	struct timeval tim1, tim2;
	long long start, end;

	
	/* Initialize the sender */
	if ((res = init(timer_handler, &timer)))
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

	return 0;
}
