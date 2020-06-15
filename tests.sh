#!/bin/bash

clear
make clean
echo 'NFUA' >> output.txt
make qemu-nox SELCTION=NFUA  >> output.txt
sanity
sleep 10
killall qemu-system-i38
cat output.txt
# open new termianl
