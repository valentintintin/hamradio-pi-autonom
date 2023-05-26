#!/bin/sh

echo "Envoi position APRS"

gpio write 3 0
gpio write 0 1

/home/valentin/softs/ax25beacon/ax25beacon -s F4HVV-1 -d APFD38 -p F4HVV,WIDE1-1,WIDE2-1 -t / -c I 45.174651 5.677257 440 "Hamnet 44.151.38.220 - $(tail -n 1 /home/valentin/logs/mpptChgD.log) - $(tail -n 1 /home/valentin/logs/temperature.log)"

gpio write 0 0
gpio write 3 1
