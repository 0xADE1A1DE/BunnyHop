#!/bash

make
mkdir results/
mkdir results/idle/

rm -f results/idle/*

#### Change me to change the pinned core
cpu0=1

for i in {0..50} # num_of_nops
do
  for j in {0..20} # depth
  do
    taskset -c $cpu0 ./bh $j $i >> results/idle/nop$i.txt
  done
  echo $i
done
