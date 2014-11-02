#include "utils.h"

static char *buf;
static unsigned char *bits;
static struct backend bck;

static void timer_handler(int signal)
{
	printf("S: Timer expired\n");
	bck.expired = 1;
}

static void sync_timer_handler(int signal)
{
	printf("S: Sync timer expired\n");
	fflush(stdout);
	bck.sync_expired = 1;
}


/* Initialize the sender */
static int init_sender(char *file_name, const int bytes)
{
	int res = 0, count;

	/* Read data to be sent from file */
	if ((count = read_from_file(file_name, &buf, bytes)) < 0)
		return res;
	printf("Buffer to send is %s\n", buf);


	/* Transform bytes to bits */
	if ((res = bytes_to_bits(buf, &bits, bytes)))
		return res;

	/* Init the backend component */
	bck.timer_handler = timer_handler;
	bck.sync_timer_handler = sync_timer_handler;
	if ((res = init(&bck)))
		return res;

	return res;
}


int main(int argc, char **argv)
{
	int res = 0;

	if (argc < 3) {
		printf("Usage: %s file_name nr_chars\n", argv[0]);
		return 1;
	}

	if ((res = init_sender(argv[1], atoi(argv[2]))))
		return res;

	printf("S: All initiated, waiting for sync to start\n");

	//send(1, &bck);
	//send(1, &bck);
	//send(0, &bck);

	//printf("Sync timer %d\n", start_sync_timer(&bck));
	//printf("Error %s\n", strerror(errno));
	sleep(10);

	free(buf);
	free(bits);
	return 0;
}
