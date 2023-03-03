#!/bin/bash

cc victim.c -lassemblyline -o victim
objdump -D victim > v.asm
cat v.asm | grep "<access_variable>:"
cc spy.c -lassemblyline -Dm_set=13 -o spy
