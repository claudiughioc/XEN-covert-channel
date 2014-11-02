#include "utils.h"

static char *buf;
static unsigned char *bits;
static struct backend bck;

static void timer_handler(int signal)
{
	printf("Timer expired\n");
	bck.expired = 1;
}

/* Initialize the sender */
static int init_sender(char *file_name, const int bytes)
{
	int res = 0, count;

	/* Init the backend component */
	bck.timer_handler = timer_handler;
	bck.expired = 0;
	if ((res = init(&bck)))
		return res;

	/* Read data to be sent from file */
	if ((count = read_from_file(file_name, &buf, bytes))< 0)
		return res;
	printf("Buffer to send is %s\n", buf);


	/* Transform bytes to bits */
	if ((res = bytes_to_bits(buf, &bits, bytes)))
		return res;

	return res;
}


int main(int argc, char **argv)
{
	int res = 0;
	struct timeval tim1, tim2;
	long long start, end;

	if (argc < 3) {
		printf("Usage: %s file_name nr_chars\n", argv[0]);
		return 1;
	}

	if ((res = init_sender(argv[1], atoi(argv[2]))))
		return res;


	gettimeofday(&tim1, NULL);
	send(1, &bck);
	send(1, &bck);
	send(0, &bck);
	gettimeofday(&tim2, NULL);
	start = tim1.tv_sec * 1000000ULL + tim1.tv_usec;
	end = tim2.tv_sec * 1000000ULL + tim2.tv_usec;
	printf("Sender started at %lld, finished at %lld\n", start, end);

	printf("All sent\n");

	free(buf);
	free(bits);
	return 0;
}
