#!/bin/sh

echo VBat : $(tail -n 1 /home/valentin/logs/mpptChgD.log | cut -d' ' -f5)mV IBat : $(tail -n 1 /home/valentin/logs/mpptChgD.log | cut -d' ' -f6)mA VSol : $(tail -n 1 /home/valentin/logs/mpptChgD.log | cut -d' ' -f2)mV ISol : $(tail -n 1 /home/valentin/logs/mpptChgD.log | cut -d' ' -f3)mA Charge : $(tail -n 1 /home/valentin/logs/mpptChgD.log | cut -d' ' -f7)mA Temp : $(tail -n 1 /home/valentin/logs/temperature.log | cut -d' ' -f11)
