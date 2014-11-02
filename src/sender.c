#include "utils.h"

struct sender_t {
	char *buf;
	int buf_p;

	unsigned char *bits;		// buffer with the bits to send
	int bits_p;			// start of next frame, bits sent
	int size;

	unsigned char frame[FRAME_TOTAL_SIZE];
	int frame_p;			// bits sent inside a frame
	char frame_no;		// frame sequence number, starts at 1
};

static struct backend bck;
static struct sender_t sender;

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
	sender.size = bytes;
	sender.frame_no = 1;
	if ((count = read_from_file(file_name, &sender.buf, bytes)) < 0)
		return res;
	printf("Buffer to send is %s\n", sender.buf);


	/* Transform bytes to bits */
	if ((res = bytes_to_bits(sender.buf, &sender.bits, bytes)))
		return res;

	/* Init the backend component */
	bck.timer_handler = timer_handler;
	bck.sync_timer_handler = sync_timer_handler;
	if ((res = init(&bck)))
		return res;

	return res;
}

static int build_frame(void)
{
	int res = 0, idx = 1, i;
	unsigned char frame[FRAME_TOTAL_SIZE];
	unsigned char *frame_no_bits;

	printf("Creating frame %d\n", (int)sender.frame_no);
	/* Start bit */
	frame[0] = (int)1;

	/* Add the sequence number */
	if ((res = bytes_to_bits(&sender.frame_no, &frame_no_bits, 1))) {
		printf("Error transforming seq number to bits\n");
		return res;
	}
	memcpy(&frame[1], frame_no_bits, FRAME_SEQ_SIZE);
	idx += FRAME_SEQ_SIZE;

	/* Add the data */
	memcpy(&frame[idx], &sender.bits[sender.bits_p], FRAME_SIZE);
	idx += FRAME_SIZE;

	/* Add the CRC code */
	idx += CRC_SIZE;

	/* Stop bit */
	frame[idx] = (int)1;

	free(frame_no_bits);
	sender.frame_no++;

	for (i = 0; i < FRAME_TOTAL_SIZE; i++)
		printf("%d, ", frame[i]);

	return res;
}


int main(int argc, char **argv)
{
	int res = 0;
	unsigned long zero_work, one_work;

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
	
	/* Wait for the sender to synchronize with the receiver */
	while (!bck.sync_expired);
	calibrate(&bck, &zero_work, &one_work, 1);
	printf("S: zero %lu, one %lu\n", zero_work, one_work);

	while (sender.bits_p < 8 * sender.size) {
		build_frame();
		sender.bits_p += FRAME_SIZE;
	}



	printf("S: is out\n");
	free(sender.buf);
	free(sender.bits);
	return 0;
}
