#!/bin/bash

make

for i in {1..60}
do
  ./bh $i
done
