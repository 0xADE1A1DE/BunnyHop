#!/bin/bash

make

for i in {1..20}
do
  taskset -c 1 ./bh $i 0
done
