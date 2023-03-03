The code under this folder explores the impact of temporal restrictions on instruction prefetcher (Sec 3.5)


## Note
- In Sec 3.5, we show that the instruction prefetcher fetches one line per cycle 
and it is shared between hyperthreads.  
Here, you can explore the behavior of instruction prefetcher for both single-thread and hyper-thread.

- To get lower noise, you may want to isolate one physical core with following command:  
`$ sudo vim /etc/default/grub`  
`GRUB_CMDLINE_LINUX_DEFAULT="quiet splash isolcpus=1,5 spectre_v2=on"`  
*1,5* are two logical cores on the same physical core.

- You can change the threshold for cache miss in *plot.py*. By default, it is 50.

## Compile
`$make`

## How to run
Full example results are listed in folder 'results/'.  
The experiment is conducted on i5-8265U with all mitigations on and cores are isolated.


### Test Instruction Prefetcher on single-thread
- You can execute `$bash test_idle.bash` to automatically test the IP on single-thread.  
The result is written to folder *results/idle/*
**The process is pinned to core 1 with taskset command, you may want to adjust it based on your machine configuration.**

- You can test it with command `taskset -c 1 ./bh $depth $nops` 
*$depth* is the memory block you want to test.  
*$nops* is the number of **NOP** instruction in function **op_probe**

### Test Instruction Prefetcher on hyper-thread
- You can automatically test it with `$bash test_busy.bash`.  
The script firstly executes *noise* on one logical core, and wait for three seconds to warmup that thread.  
Then it executes the *bh* to test the behavior of instruction prefetcher on the hyper-thread.  
The results are written to folder *results/busy*


### Plot graph
Execute `$python3 plot.py` will plot the graph and save it as *result.pdf*  
The expected graph is in folder *results/*
