#!/bin/sh
/sbin/modprobe -r ftdi_sio
/sbin/modprobe ftdi_sio
echo 0403 de29 > /sys/bus/usb-serial/drivers/ftdi_sio/new_id
