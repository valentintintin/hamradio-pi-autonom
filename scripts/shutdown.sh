#!/bin/bash

filename="/tmp/shutdown.txt"

while true; do
    if grep -q "1" "$filename"; then
        echo "Stop system in a minute"
        shutdown -P
	exit 0
    fi

    sleep 5
done
