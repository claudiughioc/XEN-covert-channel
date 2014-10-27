#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <errno.h>
#include <string.h>

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
	
	if ((res = set_afin()))
		printf("Error setting cpu affinity\n");

	return 0;
}
