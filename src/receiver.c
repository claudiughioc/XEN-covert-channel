#include "utils.h"

static int expired = 0;
timer_t timer;

static void timer_handler(int signal)
{
	printf("Timer expired\n");
	expired = 1;
}

static void recv(void)
{
	unsigned long x = 0;
	start_timer(&timer, &expired);

	while (!expired)
		x++;

	stop_timer(&timer, &expired);
	printf("Work %lu\n", x);
}


int main(void)
{
	int res = 0;
	struct timeval tim1, tim2;
	long long start, end;

	/* Initialize the receiver */
	if ((res = init(timer_handler, &timer)))
		return res;


	gettimeofday(&tim1, NULL);
	recv();
	recv();
	recv();
	gettimeofday(&tim2, NULL);

	start = tim1.tv_sec * 1000000ULL + tim1.tv_usec;
	end = tim2.tv_sec * 1000000ULL + tim2.tv_usec;
	printf("Receiver started at %lld, finished at %lld\n", start, end);
	printf("All received\n");

	return res;
}
