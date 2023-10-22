cd /home/debian/devmem2

./devmem2 0x47401c60 b 0x00
sleep 1
echo "usb1" > /sys/bus/usb/drivers/usb/unbind
sleep 20
echo "usb1" > /sys/bus/usb/drivers/usb/bind
sleep 1
./devmem2 0x47401c60 b 0x01
