#!/bin/sh

echo "Stop $(date)" >> /home/valentin/logs/state.log

gpio write 0 0 # PTT
gpio write 2 1 # Relay 1 off (borne WIFI)
gpio write 3 1 # Relay 2 off (radio)
