#!/bin/sh

/opt/vc/bin/tvservice -o
#/home/valentin/softs/uhubctl/uhubctl -a off -l 1-1.1 -p 2,3

/home/valentin/bin/timeRtc.sh

echo "Start $(date)" >> /home/valentin/logs/state.log

gpio mode 0 out
gpio write 0 0 # PTT off

gpio mode 2 out
gpio write 2 0 # Borne Wifi on

gpio mode 3 out
gpio write 3 1 # Radio off
