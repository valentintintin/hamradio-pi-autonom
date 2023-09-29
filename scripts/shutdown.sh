#!/bin/bash

if [ ! -e "/tmp/shutdown.txt" ]; then
    exit 1
fi;

if grep -q "1" "/tmp/shutdown.txt"; then
    echo "Stop system"
    /sbin/halt
fi

exit 0
