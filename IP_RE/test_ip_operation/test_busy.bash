#!/bash

make
cc noise.c -o noise

mkdir results
rm -f results/busy/*

cpu0=1
cpu1=5

taskset -c $cpu1 ./noise & noise_id=$! 
sleep 3


for i in {0..50} # num_of_nops
do
  for j in {0..20} # depth
  do
    taskset -c $cpu0 ./bh $j $i >> results/busy/nop$i.txt
  done
  echo $i
done

kill $noise_id
