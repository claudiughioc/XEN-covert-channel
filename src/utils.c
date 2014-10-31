#include "utils.h"

int start_timer(timer_t *timer, int *expired)
{
	struct itimerspec its;

	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = TIME_NSEC;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = TIME_NSEC;
	*expired = 0;

	return timer_settime(*timer, 0, &its, NULL);
}

int stop_timer(timer_t *timer, int *expired)
{
	struct itimerspec its;

	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	*expired = 0;

	return timer_settime(*timer, 0, &its, NULL);
}

static int set_afin(void)
{
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(1, &mask);
	return sched_setaffinity(0, sizeof(mask), &mask);
}

int init(timer_t *timer, void timer_handler(int))
{
	int res = 0;
	struct sigevent sevp;
	struct sigaction sa;

	/* Set process afinity */
	if ((res = set_afin()))
		printf("Error setting cpu affinity\n");

	/* Set the sigalrm handler */
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = timer_handler;
	if ((res = sigaction(SIGALRM, &sa, NULL))) {
		printf("Error setting the signal handler: %s\n", strerror(errno));
		return res;
	}


	/* Create the timer */
	sevp.sigev_notify = SIGEV_SIGNAL;
	sevp.sigev_signo = SIGALRM;
	if ((res = timer_create(CLOCK_MONOTONIC, &sevp, timer))) {
		printf("Error creating the timer %s\n", strerror(errno));
		return res;
	}

	return res;
}
