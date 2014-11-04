#include "utils.h"

static int running = 1;
static int tf = 0;
static work_id state;

static struct backend bck;
static struct worker sender;

static void timer_handler(int signal)
{
	//printf("S: Timer expired\n");
	fflush(stdout);
	bck.expired = 1;
}

/* Initialize the sender */
static int init_sender(char *file_name, const int bytes)
{
	int res = 0, count;
	unsigned long zero_work, one_work;

	/* Read data to be sent from file */
	sender.size = bytes;
	sender.frame_no = 0;
	if ((count = read_from_file(file_name, &sender.buf, bytes)) < 0)
		return res;
	printf("Buffer to send is %s\n", sender.buf);


	/* Transform bytes to bits */
	if ((res = bytes_to_bits(sender.buf, &sender.bits, bytes)))
		return res;

	/* Init the backend component */
	bck.timer_handler = timer_handler;
	if ((res = init(&bck)))
		return res;

	/* Wait for the sender to synchronize with the receiver */
	while (!bck.expired);
	calibrate(&bck, &zero_work, &one_work, 1);
	printf("S: zero %lu, one %lu\n", zero_work, one_work);
	sender.threshold = (one_work + zero_work) / 2;

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
	idx += FRAME_CRC_SIZE;

	/* Stop bit */
	frame[idx] = (int)1;

	free(frame_no_bits);

	memcpy(sender.frame, frame, FRAME_TOTAL_SIZE);

	return res;
}

static int build_info_frame(void)
{
	int res = 0, idx = 1, i;
	unsigned char frame[FRAME_TOTAL_SIZE];
	unsigned char *aux;
	char size = (char)sender.size;

	/* Start bit */
	frame[0] = (int)1;

	/* Add the size of the file */
	if ((res = bytes_to_bits(&size, &aux, 1))) {
		printf("Error transforming seq number to bits\n");
		return res;
	}
	memcpy(&frame[1], aux, FRAME_SEQ_SIZE);
	free(aux);
	idx += FRAME_SEQ_SIZE;

	/* Stop bit */
	frame[idx] = (int)1;

	printf("Info frame\n");
	for (i = 0; i < FRAME_SEQ_SIZE + 2; i++)
		printf("%d, ", frame[i]);

	memcpy(sender.frame, frame, FRAME_SEQ_SIZE + 2);
	sender.to_trans = FRAME_SEQ_SIZE + 2;
	sender.trans = 0;

	return res;
}


/* Check if ACK is OK */
static int check_ack(void)
{
	int res = 1;
	char *byte;

	/* Get frame sequence number from ACK */
	if ((res = bits_to_bytes(&sender.frame[1], &byte, 8))) {
		printf("Error converting bits to bytes\n");
		return 0;
	}
	printf("S: ACK got sequence number %d, seq is %d\n",
			(int)(*byte), sender.frame_no);

	/* Check if the sequence number corresponds to the senders */
	if ((int)(*byte) != sender.frame_no)
		return 0;

	printf("S: ACK OK\n");
	free(byte);
	return 1;
}


/* Prepare sender for the next state */
static int check_progress(void)
{
	int res = 0;

	switch (state) {
	case SEND_INFO:
		if (sender.trans)
			return res;

		printf("Sending init info, size of file is %d\n",
				sender.size);
		if ((res = build_info_frame())) {
			printf("Unable to build info frame\n");
			return res;
		}
		break;

	case SEND:
		if (sender.trans)
			return res;

		printf("S: Going to send the next frame %d\n", sender.frame_no);
		if ((res = build_frame())) {
			printf("Unable to build frame\n");
			return res;
		}
		print_frame(sender);
		fflush(stdout);
		break;

	case RECV_ACK:
		if (sender.trans)
			return res;

		printf("S: receiving ACK\n");
		break;

	default:
		printf("Unknown state\n");
	}

	return res;
}


int main(int argc, char **argv)
{
	int res = 0;
	unsigned long work;

	if (argc < 3) {
		printf("Usage: %s file_name nr_chars\n", argv[0]);
		return 1;
	}

	if ((res = init_sender(argv[1], atoi(argv[2]))))
		return res;

	printf("S: All initiated\n");


	state = SEND_INFO;
	printf("S: Going to run now\n");
	while (running) {
		printf("\nS: Time frame is %d\n", tf);
		bck.expired = 0;


		/* Prepare what to do next */
		if ((res = check_progress())) {
			printf("Unable to prepare sender\n");
			return res;
		}

		/* Do something until the timer expires */
		switch (state) {
		case SEND_INFO:
			printf("S: Work is SEND_INFO\n");

			send(sender.frame[sender.trans], &bck);
			sender.trans++;

			/* Finish sending info, switch to next state */
			if (sender.trans == sender.to_trans) {
				sender.trans = 0;
				sender.to_trans = FRAME_TOTAL_SIZE;
				state = SEND;
			}
			break;


		case SEND:
			printf("S: Work is SEND\n");
			send(sender.frame[sender.trans], &bck);
			sender.trans++;

			/* Finish sending frame, wait for ACK */
			if (sender.trans == sender.to_trans) {
				printf("_____________STOP_________\n");
				sender.trans = 0;
				sender.to_trans = FRAME_ACK_SIZE;
				state = RECV_ACK;
			}
			break;

		case RECV_ACK:
			printf("S: Work is RECV_ACK\n");
			work = recv(&bck);
			fill_frame(work, &sender);

			/* Finished receiving ACK */
			if (sender.trans == sender.to_trans) {
				if (check_ack())
					sender.frame_no++;
				else
					printf("S: ACK WRONG, resending %d\n",
							sender.frame_no);

				sender.trans = 0;
				sender.to_trans = FRAME_TOTAL_SIZE;
				state = SEND;
			}
			break;
		case WAIT:
			printf("Work is WAIT\n");
			break;
		default:
			printf("NO WORK SPECIFIED. THAT'S WRONG\n");
		}

		while (!bck.expired);

		tf++;
		if (sender.frame_no * FRAME_BYTES > sender.size)
			break;
		fflush(stdout);
	}



	printf("S: is out\n");
	free(sender.buf);
	free(sender.bits);
	stop_timer(&bck);
	return 0;
}
