#!/bin/bash

echo "Start test"
./receiver &
./sender test 10 &
