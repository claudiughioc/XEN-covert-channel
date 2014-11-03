#include "utils.h"

static int running = 1;
static int tf = 0;
static work_id state;

static struct worker receiver;
static struct backend bck;

static void timer_handler(int signal)
{
	//printf("R: Timer expired\n");
	fflush(stdout);
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

static int build_ack_frame()
{
	int res = 0, idx = 1;
	unsigned char frame[FRAME_TOTAL_SIZE];
	unsigned char *aux;
	char seq = (char)receiver.frame_no;

	/* Start bit */
	frame[0] = (int)1;

	/* Add the size of the file */
	if ((res = bytes_to_bits(&seq, &aux, 1))) {
		printf("Error transforming seq number to bits\n");
		return res;
	}
	memcpy(&frame[1], aux, FRAME_SEQ_SIZE);
	free(aux);
	idx += FRAME_SEQ_SIZE;

	/* Stop bit */
	frame[idx] = (int)1;

	memcpy(receiver.frame, frame, FRAME_ACK_SIZE);
	receiver.to_trans = FRAME_ACK_SIZE;
	receiver.trans = 0;

	return res;

}

static void store_info(void)
{
	char *seq, *byte, *crc;

	switch (state) {
	case RECV_INFO:
		if (bits_to_bytes(&receiver.frame[1], &byte, 8)) {
			printf("Error converting bits to bytes\n");
			return;
		}
		printf("Will have to transfer %d bytes\n", (int)(*byte));

		/* Allocate space for receiver buffers */
		receiver.size = (int)(*byte);
		receiver.buf = malloc(receiver.size * sizeof(char));
		receiver.bits = malloc(8 * receiver.size * sizeof(char));

		free(byte);
		break;

	case RECV:
		/* Get sequence number */
		if (bits_to_bytes(&receiver.frame[1], &seq, FRAME_SEQ_SIZE)) {
			printf("Error converting bits to bytes\n");
			return;
		}
		printf("R: Sequence number is %d\n", (int)(*seq));

		/* Get payload */
		if (bits_to_bytes(&receiver.frame[1 + FRAME_SEQ_SIZE],
					&byte, FRAME_SIZE)) {
			printf("Error converting bits to bytes\n");
			return;
		}

		/* Get crc */
		if (bits_to_bytes(&receiver.frame[1 + FRAME_SEQ_SIZE + FRAME_SIZE],
					&crc, FRAME_CRC_SIZE)) {
			printf("Error converting bits to bytes\n");
			return;
		}

		/* TODO: check CRC, copy data to bits if ok */
		receiver.frame_no = (int)(*seq);

		free(byte);
		free(seq);
		free(crc);
		break;

	default:
		break;
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

	case RECV:
		/* During transfer */
		if (receiver.trans)
			return res;

		printf("R: Receiving frame\n");
		break;
	
	case SEND_ACK:
		if (receiver.trans)
			return res;

		printf("R: Sending ACK\n");
		if ((res = build_ack_frame())) {
			printf("Unable to build ack frame\n");
			return res;
		}
		printf("R: ACK FRAME for %d\n", receiver.frame_no);
		print_frame(receiver);

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
			fill_frame(work, receiver);

			/* Finish receiving info, switch to next state */
			if (receiver.trans == receiver.to_trans) {
				print_frame(receiver);
				store_info();

				receiver.trans = 0;
				receiver.to_trans = FRAME_TOTAL_SIZE;
				state = RECV;
			}
			break;

		case RECV:
			printf("R:Work is RECV, trans %d, to_trans%d \n",
					receiver.trans, receiver.to_trans);
			work = recv(&bck);
			fill_frame(work, receiver);

			/* Finished receiving, send ACK */
			if (receiver.trans == receiver.to_trans) {
				printf("--------HALT------");
				print_frame(receiver);
				store_info();
				printf("After storing\n");
				fflush(stdout);

				receiver.trans = 0;
				receiver.to_trans = FRAME_ACK_SIZE;
				state = SEND_ACK;
			}
			printf("R:out of recv\n");
			fflush(stdout);
			break;

		case SEND_ACK:
			printf("R:Work is SEND_ACK\n");
			send(receiver.frame[receiver.trans], &bck);
			receiver.trans++;

			if (receiver.trans == receiver.to_trans) {
				printf("-----------STOP ACK------------\n");
			}
			break;

		case WAIT:
			printf("R:Work is WAIT\n");
			break;

		default:
			printf("NO WORK SPECIFIED. THAT'S WRONG\n");
		}

		while (!bck.expired);

		tf++;
		if (!running)
			break;
		fflush(stdout);
	}


	printf("R is out\n");
	free(receiver.buf);
	free(receiver.bits);
	stop_timer(&bck);

	return res;
}
