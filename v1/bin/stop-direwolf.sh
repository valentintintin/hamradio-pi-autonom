#!/bin/bash

if ! [ $(id -u) = 0 ]; then
   echo "The script need to be run as root." >&2
   exit 1
fi

#echo "Kill node"
#killall node
echo "Kill direwolf"
killall direwolf
echo "Kill mkiss"
killall mkiss
echo "Kill kissattach"
killall kissattach
#echo "Kill node twice"
#killall node #because node need twice kill

gpio write 3 1 # Radio Off
gpio write 0 0 # PTT Off
