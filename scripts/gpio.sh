#!/bin/sh -e

sleep 15

# turn off LEDs 1 - 3, only 0 will be on
#echo 0 > /sys/class/leds/beaglebone:green:usr1/brightness
#echo 0 > /sys/class/leds/beaglebone:green:usr2/brightness
#echo 0 > /sys/class/leds/beaglebone:green:usr3/brightness

# configure UART
config-pin p9.24 uart
config-pin p9.26 uart
stty -F /dev/ttyS1 115200

# cpu
cpufreq-set --governor conservative

# rtc
echo ds3231 0x68 > /sys/class/i2c-adapter/i2c-2/new_device
hwclock -s -f /dev/rtc1
