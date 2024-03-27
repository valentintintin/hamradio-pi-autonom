date

if pgrep -f "rsync -rvz" >/dev/null; then
  echo "Rsync is already running. Exiting."
  exit 1
fi

rsync -rvz --size-only --progress /home/debian/docker/storage/ rsync://192.168.1.254:873/opi
#rsync -avz --no-perms --no-owner --progress --no-group no-times --size-only /home/debian/docker/storage/ rsync://192.168.1.254:873/opi
