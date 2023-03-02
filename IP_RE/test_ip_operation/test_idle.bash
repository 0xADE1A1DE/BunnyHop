#!/bash

make

for i in {0..50} # num_of_nops
do
  for j in {0..20} # depth
  do
    taskset -c 1 ./bh $j $i >> results/nop$i.txt
  done
  echo $i
done
