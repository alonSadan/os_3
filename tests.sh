#!/bin/bash

clear
make clean
rm -r output.txt
touch output.txt
echo 'NFUA' >>output.txt
/bin/sh -ec 'make qemu-nox SELCTION=NFUA  >> output.txt;'
# sleep 10; sanity 
killall qemu-system-i38
cat output.txt
# open new termianl


# terminate qemu
# echo 'LAPA' >> output.txt
# make qemu-nox SELCTION=LAPA
# sanity >> output
# terminate qemu
