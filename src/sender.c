#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

#define LARGE_PRIME		492876847
#define TIME_NSEC		50000000

int expired = 0;
timer_t timer;

static void timer_handler(int signal)
{
	printf("Timer expired\n");
	expired = 1;
}

static int start_timer(void)
{
	struct itimerspec its;

	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = TIME_NSEC;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = TIME_NSEC;
	expired = 0;

	return timer_settime(timer, 0, &its, NULL);
}

static int stop_timer(void)
{
	struct itimerspec its;

	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	expired = 0;

	return timer_settime(timer, 0, &its, NULL);
}

static void send_one(void)
{
	unsigned long long x = 1;
	printf("Sending one\n");

	start_timer();

	while (!expired) {
		x *= LARGE_PRIME;
	}

	stop_timer();
	printf("Finished work\n");
}

static void send_zero(void)
{

	printf("Sending zero\n");
}


static void send(int val)
{
	switch (val) {
	case 1:
		send_one();
		break;
	case 0:
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

int main(void)
{
	int res = 0;
	struct sigevent sevp;
	struct sigaction sa;
	
	if ((res = set_afin()))
		printf("Error setting cpu affinity\n");


	/* Set the sigalrm handler */
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &timer_handler;
	sigaction (SIGALRM, &sa, NULL);


	/* Create the timer */
	sevp.sigev_notify = SIGEV_SIGNAL;
	sevp.sigev_signo = SIGALRM;
	if ((res = timer_create(CLOCK_MONOTONIC, &sevp, &timer))) {
		printf("Error creating the timer %s\n", strerror(errno));
		return res;
	}


	send(1);
	while(1);
	return 0;
}
