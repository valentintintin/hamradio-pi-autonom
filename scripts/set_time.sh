#!/bin/bash

date

time=$(curl -f http://localhost/states/datetime) && echo "Set time to $time" && date -s "$time"
