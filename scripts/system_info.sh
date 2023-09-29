#!/bin/bash

# Get raw CPU percentage
cpu_percentage=$(top -bn1 | grep "%Cpu(s)" | awk '{printf "%.0f", $2}')

# Get RAM percentage
ram_percentage=$(free | grep Mem | awk '{printf "%.0f", (1 - $7/$2) * 100}')

# Get disk usage percentage
disk_percentage=$(df -h / | awk 'NR==2 {print $5}' | cut -d'%' -f1)

# Get uptime in seconds
uptime_seconds=$(awk '{printf "%.0f", $1}' /proc/uptime)

# Build JSON output
json="{ \"cpu\": $cpu_percentage, \"ram\": $ram_percentage, \"disk\": $disk_percentage, \"uptime\": $uptime_seconds }"

# Output the JSON
echo "$json"

curl -X POST -H "Content-Type: application/json" -d "$json" http://localhost:$1/system_info
