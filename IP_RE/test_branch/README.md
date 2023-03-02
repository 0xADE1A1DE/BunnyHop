The code under this folder explores how BTB affects the instruction prefetching.

## Notes
- We define *THRESHOLD* to distinguish cache miss and cache hit. The vaule is various from processors. If you are not sure with the value of THRESHOLD, you can just comment that line and the program will prints the raw timing result.

- You can change two lines in *main.c* under *change me* to play more.

## Compile
`$make`

## How to run
- You can run the program with command `./bh $depth`  
*depth* stands for which cache line following the *branch_probe* is tested.  

- Or you can run `bash experiment.bash` to automatically test a range of *depth*

## Expected Result
You will get reuslt similar to the one below:  
`Test Depth  1, cached`  
It states the which cache line is tested to be or not to be in the cache. It further tells the cache status.  

You can find full expected results under folder /results.
The result is tesed on machine i5-8265U with spectre-v2 mitigation on.

## About the code.
The provided code tests two properties of the instruction prefetcher:
- A recod in the BTB trains the instruction prefetcher to prefetch the branch target
- The instruction prefetcher look several steps ahead. Specifically, the instruction prefetcher keeps fetching blocks until it realises the prediction is incorrect.

In the main.c file, we implement function *genjmps* to contain chained branches.  
To increase the *window* of fetching on the predicted path, the *branch_probe* function contains 300 *NOP*s.
