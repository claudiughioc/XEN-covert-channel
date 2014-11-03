#include "utils.h"

static int running = 1;
static int tf = 0;
static work_id state;

static struct worker receiver;
static struct backend bck;

static void timer_handler(int signal)
{
	//printf("R: Timer expired\n");
	bck.expired = 1;
}


/* Initialize the receiver */
static int init_receiver(void)
{
	int res = 0;
	unsigned long zero_work, one_work;

	/* Init the backend component */
	bck.timer_handler = timer_handler;
	if ((res = init(&bck)))
		return res;

	/* Wait for the receiver to synchronize with the sender */
	while (!bck.expired);
	calibrate(&bck, &zero_work, &one_work, 0);
	printf("R: zero %lu, one %lu\n", zero_work, one_work);
	receiver.threshold = (one_work + zero_work) / 2;

	return res;
}

static void fill_frame(unsigned long work)
{
	if (work < receiver.threshold) {
		printf("R: Got a one with %lu\n", work);
		receiver.frame[receiver.trans++] = (int)1;
	} else {
		printf("R: Got a zero with %lu\n", work);
		receiver.frame[receiver.trans++] = (int)0;
	}
}

static int check_progress(void)
{
	int res = 0;

	switch (state) {
	case RECV_INFO:
		if (receiver.trans)
			return res;

		printf("R: Receiving init info\n");
		receiver.trans = 0;
		receiver.to_trans = FRAME_SEQ_SIZE + 2;
		break;

	default:
		printf("Unknown state\n");
	}

	return res;
}


int main(void)
{
	int res = 0;
	unsigned long work;

	if ((res = init_receiver()))
		return res;

	printf("R: All initiated\n");


	state = RECV_INFO;
	printf("Going to run now\n");
	while (running) {
		printf("\nR: Time frame is %d\n", tf);
		bck.expired = 0;


		/* Prepare what to do next */
		if ((res = check_progress())) {
			printf("Unable to prepare receiver\n");
			return res;
		}

		/* Do something until the timer expires */
		switch (state) {
		case RECV_INFO:
			printf("R:Work is RECV_INFO\n");

			work = recv(&bck);
			fill_frame(work);

			if (receiver.trans == receiver.to_trans) {
				state = RECV;
			}
			break;

		case RECV:
			printf("R:Work is RECV\n");
			print_frame(receiver);
			break;

		case WAIT:
			printf("R:Work is WAIT\n");
			break;

		default:
			printf("NO WORK SPECIFIED. THAT'S WRONG\n");
		}

		printf("Wait for tick\n");
		while (!bck.expired);
		printf("Go to next time frame\n");

		tf++;
		if (!running)
			break;
		fflush(stdout);
	}


	printf("R is out\n");
	stop_timer(&bck);

	return res;
}
