#!/usr/bin/env bash

if [ "$#" -ne 3 ]; then
    echo "Usage: ./program_device.sh <callsign> <grid> <pico-w-usb-port>"
    exit 1
fi

echo $1

git checkout Amplified-WSPR-Beacon-v2/firmware/firmware.ino  # CAREFUL!
sed -i "s/VU3CER/$1/g" Amplified-WSPR-Beacon-v2/firmware/firmware.ino
sed -i "s/MK68/$2/g" Amplified-WSPR-Beacon-v2/firmware/firmware.ino
make TARGET=$3
