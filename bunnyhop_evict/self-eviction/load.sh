#!/bin/bash
sudo dmesg -C
sudo rmmod cdc.ko
make
sudo insmod cdc.ko
dmesg
sudo cat /sys/module/cdc/sections/.text
