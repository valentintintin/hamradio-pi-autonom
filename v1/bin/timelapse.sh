#!/bin/bash

DATE=$(date +"%Y-%m-%d")
DIR="/home/valentin/projects/timelapse/$DATE"

[ ! -d "$DIR" ] && mkdir -p "$DIR"
(ssh f4hvv@44.151.38.181 -T "[ ! -d \"/home/f4hvv/pixelPi/timelapse/$DATE\" ] && mkdir -p \"/home/f4hvv/pixelPi/timelapse/$DATE\"") &

fswebcam -c /home/valentin/fswebcam.conf && scp "$DIR/$(ls -t1 $DIR |  head -n 1)" f4hvv@44.151.38.181:/home/f4hvv/pixelPi/timelapse/$DATE/
