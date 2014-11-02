#include "utils.h"

static struct backend bck;

static void timer_handler(int signal)
{
	printf("Timer expired\n");
	bck.expired = 1;
}

/* Initialize the receiver */
static int init_receiver(void)
{
	int res = 0;

	/* Init the backend component */
	bck.timer_handler = timer_handler;
	bck.expired = 0;
	if ((res = init(&bck)))
		return res;

	return res;
}

int main(void)
{
	int res = 0;
	struct timeval tim1, tim2;
	long long start, end;

	if ((res = init_receiver()))
		return res;

	gettimeofday(&tim1, NULL);
	printf("Received %lu \n", recv(&bck));
	printf("Received %lu \n", recv(&bck));
	printf("Received %lu \n", recv(&bck));
	gettimeofday(&tim2, NULL);

	start = tim1.tv_sec * 1000000ULL + tim1.tv_usec;
	end = tim2.tv_sec * 1000000ULL + tim2.tv_usec;
	printf("Receiver started at %lld, finished at %lld\n", start, end);
	printf("All received\n");

	return res;
}
