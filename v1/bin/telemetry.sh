#!/bin/bash

echo $(tail -n 1 /home/valentin/logs/mpptChgD.log) - $(/home/valentin/bin/temperature.sh)
echo $(tail -n 1 /home/valentin/logs/mpptChgD.log) - $(tail -n 1 /home/valentin/logs/temperature.log) | ssh f4hvv@44.151.38.181 -T "cat >> /home/f4hvv/pixelPi/logs.log" > /dev/null 2>&1
