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

int start_timer(timer_t *timer, int *expired);
int stop_timer(timer_t *timer, int *expired);
int init(timer_t *timer, void timer_handler(int));
