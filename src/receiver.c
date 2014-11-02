#include "utils.h"

static struct backend bck;

static void timer_handler(int signal)
{
	printf("R: Timer expired\n");
	bck.expired = 1;
}

static void sync_timer_handler(int signal)
{
	printf("R: Sync timer expired\n");
	fflush(stdout);
	bck.sync_expired = 1;
}

/* Initialize the receiver */
static int init_receiver(void)
{
	int res = 0;
	timer_t timer;

	/* Init the backend component */
	bck.timer = &timer;
	bck.timer_handler = timer_handler;
	bck.sync_timer_handler = sync_timer_handler;
	if ((res = init(&bck)))
		return res;

	return res;
}

int main(void)
{
	int res = 0;
	unsigned long zero_work, one_work;

	if ((res = init_receiver()))
		return res;

	//recv(&bck);
	//recv(&bck);
	//recv(&bck);

	printf("R: All initiated, waiting for sync to start\n");

	/* Wait for the receiver to synchronize with the sender */
	while (!bck.sync_expired);
	calibrate(&bck, &zero_work, &one_work, 0);
	printf("R: zero %lu, one %lu\n", zero_work, one_work);

	printf("R is out\n");

	return res;
}
