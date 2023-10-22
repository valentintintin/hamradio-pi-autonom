#!/bin/bash

STATUS=$(i2cget -y 2 0x12 2 w)
STATUS=$(( (($STATUS & 0xFF) << 8) | (($STATUS >> 8) & 0xFF) ))
echo STATUS = $STATUS

VS=$(i2cget -y 2 0x12 6 w)
VS=$(( (($VS & 0xFF) << 8) | (($VS >> 8) & 0xFF) ))
echo VS = $VS mV

IS=$(i2cget -y 2 0x12 8 w)
IS=$(( (($IS & 0xFF) << 8) | (($IS >> 8) & 0xFF) ))
echo IS = $IS mA

VB=$(i2cget -y 2 0x12 10 w)
VB=$(( (($VB & 0xFF) << 8) | (($VB >> 8) & 0xFF) ))
echo VB = $VB mV

IB=$(i2cget -y 2 0x12 12 w)
IB=$(( (($IB & 0xFF) << 8) | (($IB >> 8) & 0xFF) ))
echo IB = $IB mA

IC=$(i2cget -y 2 0x12 14 w)
IC=$(( (($IC & 0xFF) << 8) | (($IC >> 8) & 0xFF) ))
ICMinus=$(( $IC & 0x8000 ))
if [[ $ICMinus > 0 ]]; then
    IC=$(( -(0x8000 - ($IC & 0x7FFF)) ))
fi
echo IC = $IC mA
