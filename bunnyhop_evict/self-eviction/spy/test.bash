#!/bin/bash
sudo dmesg -C

cc spy.c -Dattack_offset=4096 -Dtrain=1 -DBASE=0x40c0e38000ULL -DOFFSET=0x10ULL -lassemblyline

taskset -c 1 ./a.out 1
