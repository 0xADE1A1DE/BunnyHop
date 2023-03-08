# BunnyHop: Exploiting the Instruction Prefetcher
*Authors:* **Zhiyuan Zhang**, **Mingtian Tao**, **Sioli O'Connell**, 
**Chitchanok Chuengsatiansup**, **Daniel Genkin**, **Yuval Yarom**

*The paper is accepted in USENIX Security 2023 Fall Round*

## What is BunnyHop ?
Bunnyhop is a technique that translates the BPU prediction (BTB specifically) into cache status.
Based on this observation, we introduce three cache attack techniques, flush+reload, prime+probe and evict+time to observe BTB contents.


## Artifact Evaluation
We provide several experiments for evaluating the power of BunnyHop.  

### Essential Libraries
To start the evaluation, you will need to install two libraries: Mastik and Assemblyline. 
Both of them are available under 0XADE1A1DE repo.  
Mastik is a lightweight library that provides cache based side-channel APIs. 
In BunnyHop, we use Mastik to flush memories and measure memory accesses.  
AssemblyLine is a powerful tool that translates assembly code into binary 
and write them to the memory at a given location.  
We use AssemblyLine to generate functions to test the BTB content. 

### General system configuration
Replace the original line of *GRUB_CMDLINE_LINUX_DEFAULT* with `GRUB_CMDLINE_LINUX_DEFAULT="quiet splash isolcpus=1,7 spectre_v2=on"`. The isolated processors should be two sibling cores.  
Set the processor governor to *performance*.


### Reverse Engineer the Instruction Prefetcher
The evaluation starts with experiments on reverse engineering the Instruction Prefetcher.  
The code is under folder *IP_RE*.

#### Prefetching Depth
Under folder *test_depth* is the code that evaluates 
how many cache blocks ahead the current prefetched block is prefetched.  
You can find more information and the expected result under the folder.

#### BTB affects on the Instruction Prefetcher
Under folder *test_branch* is the code that evaluates how BTB affects the Instruction Prefetcher.  
The code we present reveals that the IP follows the content of BTB 
and it keeps prefetching multiple branch targets until the end of *speculation*.  
You can find more information and the expected result under the folder.

#### Temporal restrictions on the Instruction Prefetcher
Under folder *test_ip_operation* we investigate when the prefetching can be find to be incorrect. 
We evaluate the scenarios that the hyper-thread is either idle or busy.  
You can find more information and the expected result under the folder.

### Reverse Engineer the BTB
Under folder *BTB_RE* is the code that finds the target bits for long branch and short branches.  
In the paper, we show that a long branch stores 32 LSBs of the target address 
while a short branch stores 10 LSBs of the target address.  
The experiment we provide is to reproduce the Figure 3.  
You can find more information and the expected result under the folder.

### Break KASLR
Under folder *bunnyhop_fr* is the code that break KASLR with BunnyHop-Reload technique.  

### Prime+Probe on BTB
Undre folder *bunnyhop_pp* is the code that performs BunnyHop-Probe.  
We provide the code that evaulates the accuracy of the BunnyHop-Probe.  
You can find more information and the expected result under the folder.
