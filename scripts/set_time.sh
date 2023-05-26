#!/bin/bash

# Loop until the file "/tmp/time.txt" exists
if [ ! -e "/tmp/time.txt" ]; then
    exit 1
fi;

# Read the time from the file and set it using the "date" command
time=$(cat "/tmp/time.txt")

echo "Set time to $time"

date -s "$time"

rm /tmp/time.txt
