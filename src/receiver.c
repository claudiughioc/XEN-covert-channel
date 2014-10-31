#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define LARGE_PRIME		492876847
#define TIME_NSEC		50000000
#define NSEC_TO_USEC(x)		((x) / 1000ULL)

static int expired = 0;
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

static int set_afin(void)
{
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(1, &mask);
	return sched_setaffinity(0, sizeof(mask), &mask);
}

static void recv(void)
{
	unsigned long x = 0;
	start_timer();

	while (!expired)
		x++;

	stop_timer();
	printf("Work %lu\n", x);
}


static int init_recv(void)
{
	int res = 0;
	struct sigevent sevp;
	struct sigaction sa;
	
	if ((res = set_afin()))
		printf("Error setting cpu affinity\n");

	/* Set the sigalrm handler */
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &timer_handler;
	if ((res = sigaction(SIGALRM, &sa, NULL))) {
		printf("Error setting the signal handler: %s\n", strerror(errno));
		return res;
	}


	/* Create the timer */
	sevp.sigev_notify = SIGEV_SIGNAL;
	sevp.sigev_signo = SIGALRM;
	if ((res = timer_create(CLOCK_MONOTONIC, &sevp, &timer))) {
		printf("Error creating the timer %s\n", strerror(errno));
		return res;
	}

	return res;
}

int main(void)
{
	struct timeval tim1, tim2;
	long long start, end;

	init_recv();

	gettimeofday(&tim1, NULL);
	recv();
	recv();
	recv();
	gettimeofday(&tim2, NULL);

	start = tim1.tv_sec * 1000000ULL + tim1.tv_usec;
	end = tim2.tv_sec * 1000000ULL + tim2.tv_usec;
	printf("Receiver started at %lld, finished at %lld\n", start, end);
	printf("All received\n");

	return 0;
}
