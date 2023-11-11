#!/bin/bash

server="192.168.1.241"
port=22

case $1 in
"home")
    server="192.168.1.241"
    port=22
    ;;
"usb")
    server="192.168.7.2"
    port=22
    ;;
esac

set -e
set -x

rm -Rf published/*
cd Monitor || exit
dotnet publish -c Release --no-restore --no-self-contained --nologo --output ../published/ -r linux-arm
cd ..

rsync -e "ssh -p $port" -r --info=progress2 published/ debian@$server:/home/debian/docker/Monitor

case $2 in
"all")
    ssh debian@$server -t -p $port "cd /home/debian/docker/Monitor && docker compose build monitor && docker compose stop monitor && docker compose up -d monitor && docker system prune -f && docker compose logs -f --tail 10 monitor"
    ;;
*)
    ssh debian@$server -t -p $port "cd /home/debian/docker/Monitor && docker compose restart monitor && docker compose logs -f --tail 10 monitor"
    ;;
esac
