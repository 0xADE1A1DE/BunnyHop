
The code under this folder explores the instruction prefetching depth (Sec 3.1).

## Compile
`$make`

## How to run
- You can run the program with following command `./bh $deptch $train`.  
The first parameter determines which memory block following the branch_probe is tested.  
The second parameter determines whether branch_probe will be invoked once to write *RET* into the BTB.

- Or you can run `bash experiment.bash` to automatically run the test.  
*train* is set to 0 by default in the experiment.bash.  
Set it to 1 to train the BTB.
