#!/bin/bash
if ! [ $(id -u) = 0 ]; then
   echo "The script need to be run as root." >&2
   exit 1
fi

gpio write 0 0 # PTT Off
gpio write 3 0 # Radio on

PATH=/usr/local/sbin:/usr/local/bin:/bin:/usr/bin:/etc/ax25:/usr/local/ax25:$PATH
echo "Starting direwolf"
direwolf -t 0 -c /home/valentin/direwolf.conf -p  &
#Check if Direwolf is running
sleep 5
if [ -z "`ps ax | grep -v grep | grep direwolf`" ]; then
        echo -e "\nERROR: Direwolf did not start properly and is not running, please review direwolf.conf"
        exit 1
fi
echo "Installing one KISS connection on PTY port /tmp/kisstnc"
/usr/sbin/mkiss -s 19200 -x 1 /tmp/kisstnc > /tmp/unix98
#This creates a PTS interface like "/dev/pts/3"
export PTS0=`more /tmp/unix98 | grep -w /dev | cut -b -11`
echo "PTS0 device: $PTS0"
/usr/sbin/kissattach $PTS0 ax0  > /tmp/ax25-config.tmp
awk '/device/ { print $7 }' /tmp/ax25-config.tmp > /tmp/ax25-config1-tmp
read Device < /tmp/ax25-config1-tmp
/usr/sbin/kissparms -p ax0 -t 300 -l 30 -s 100 -r 63 -f n
ifconfig ax0 44.151.138.221/24
arp -H ax25 -s 44.151.138.220 F4HVV -i ax0
#ip route add 44.151.38.0 dev ax0 scope link
#/usr/local/bin/node /home/valentin/projects/ax25-server-nodejs/dist/app.js >> /home/valentin/logs/node-ax25.log &

pgrep direwolf || gpio write 3 1 # Relay off (radio)
