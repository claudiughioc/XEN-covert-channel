#!/bin/bash
SENDER_USER=root
SENDER_HOST=10.0.0.2
SENDER_PASSWD=student

RECEIVER_USER=root
RECEIVER_HOST=10.0.0.3
RECEIVER_PASSWD=student

DEFAULT_OUTPUT_FILE=output

if [ $# -lt 2 ]; then
	echo "Usage ./run.sh file size"
	exit 1
fi

echo "Sending file $1, size $2 from $SENDER_HOST to $RECEIVER_HOST"


# Copy executables to servers
export SSHPASS=$SENDER_PASSWD
sshpass -e scp -oBatchMode=no src/sender $SENDER_USER@$SENDER_HOST:
sshpass -e scp -oBatchMode=no test $SENDER_USER@$SENDER_HOST:
export SSHPASS=$RECEIVER_PASSWD
sshpass -e scp -oBatchMode=no src/receiver $RECEIVER_USER@$RECEIVER_HOST:

# Run sender and receiver
export SSHPASS=$RECEIVER_PASSWD
sshpass -e ssh -oBatchMode=no $RECEIVER_USER@$RECEIVER_HOST "./receiver $DEFAULT_OUTPUT_FILE &" &
export SSHPASS=$SENDER_PASSWD
sshpass -e ssh -oBatchMode=no $SENDER_USER@$SENDER_HOST "./sender $1 $2"

echo "Check the $DEFAULT_OUTPUT_FILE file at $RECEIVER_HOST for results"
