#!/bin/bash

# Get CPU percentage
cpu_percentage=$(top -bn1 | grep "%Cpu(s)" | awk '{print $2}')

# Get RAM percentage
ram_percentage=$(free | grep Mem | awk '{printf "%.2f", (1 - $7/$2) * 100}')

# Get disk usage percentage
disk_percentage=$(df -h / | awk 'NR==2 {print $5}' | cut -d'%' -f1)

# Get uptime in seconds
uptime_seconds=$(awk '{print $1}' /proc/uptime)

# Build JSON output
json_output=$(jq -n --arg cpu_percentage "$cpu_percentage" --arg ram_percentage "$ram_percentage" --arg disk_percentage "$disk_percentage" --arg uptime_seconds "$uptime_seconds" '{"cpu_percentage": $cpu_percentage, "ram_percentage": $ram_percentage, "disk_percentage": $disk_percentage, "uptime_seconds": $uptime_seconds}')

# Output the JSON
echo "$json_output"