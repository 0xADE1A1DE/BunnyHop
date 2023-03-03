The code under this folder test the number of target bits stored by long & short branches.

## Note
For detailed information on VS-BTB, please refer to Sec 4.1 and 4.2 of the paper.  
In summary, a short branch can be stored in both long and short branch BTB slots 
while the long branch can only go to the long branch slot. 
Long branch slot stores 32 LSBs of the target address while the shor branch slot sotres 10 LSBs of the target address.

## Compile
`$gcc main.c -lassemblyline -o bh`


## How to run
- You can run the experiment with command `$taskset -c 1 ./bh > result.txt`

- You can plot the graph with command `python3 plot.py` and the result is saved as result.pdf
