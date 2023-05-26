#!/bin/bash

echo "Pi $(vcgencmd measure_temp | egrep -o '[0-9]*\.[0-9]*')°C - Ext $(echo "scale=1; $(cat /sys/bus/i2c/devices/1-0068/hwmon/hwmon1/temp1_input)/1000.0" | bc)°C"
echo "$(date) Pi $(vcgencmd measure_temp | egrep -o '[0-9]*\.[0-9]*')°C - Ext $(echo "scale=1; $(cat /sys/bus/i2c/devices/1-0068/hwmon/hwmon1/temp1_input)/1000.0" | bc)°C" >> /home/valentin/logs/temperature.log

