#!/bin/bash

bash compile.bash
echo "Done compilation"

mkdir results/
rm -f overall_result.txt

## Two logical processors
cpu0=1
cpu1=5

for j in {1..100}
do
    rm -f final_result.txt
    v=$[ $RANDOM % 256 ]
    echo $v
    for i in {1..100}
    do
        taskset -c $cpu0 ./spy ./victim > results/spy.outcome$i & SPY_PID=$!

        #echo "wait and execute spy..."
        sleep 0.2

        taskset -c $cpu1 ./victim $v 

        #echo "kill spy process"
        kill -2 $SPY_PID

        cat results/spy.outcome$i >> final_result.txt
        echo $i
    done
    echo $j
    python3 plot.py $v >> overall_result.txt
done
