The code under this folder reproduce the toy example of BunnyHop-Probe in Sec 8.1

## Compile
`$bash compile.bash`

### Run
`$bash experiment.bash`

The bash script firstly compile the program.  
It then generates a random byte which is later fed to the victim process.  
The script firstly executes the spy function which is infinitly running.  
The script waits of 0.2 seconds to warmup the processor and then it invokes the victim process.  
The processes are pinned to two sibling processors.  
**You may need to change $cpu0, $cpu1 in the bash to get correct configuration**  

Overall, we generate 100 random bytes and it takes a while to run all tests.  
After all testes are done, you could run `$python3 final_analyse.py` to get an overall accuracy.

## Code
The victim firstly accesses a *memory* and then it handles the secret byte.
The victim processes a secret byte bit-by-bit.  
A branch is taken if the bit is 1.  

The spy keeps monitoring whether the *memory* is accessed.  
When the *memory* is accessed, the spy starts doing prime+probe on the target BTB cache set.

## Notes
The attack is similar to flush+reload attack where an attacker needs to configure 
the right waiting cycles.  
The waiting cycles are various from processors and cpu frequencies.  
**You can change the waiting cycles in the *delayloop()* at line 82**  
We provide a sample result in file *overall_result.txt*.  
The result is obtained on i5-8265U, with all Spectre-v2 mitigations on.

## Expected Results
We process the raw data and write the analysed data into file *overall_result.txt*.  
You will see results similar to the one below:  
0.25 0.45 0.15 0.70 0.05 0.90 1.00 0.90 8 234 234  
The first eight values represent the percentage a branch is taken among 100 tests.  
The ninth value is the number of correctly guessed bits.
The 10th value is the guessed byte while the 11th value is the secret byte.  

## Debug Program
As mentioned before, the key to do the attack is the waiting cycles and 
it takes time to find a proper window.  
To debug the code, you can set the script (experiment.bash) 
to only test one secret byte (line 13).  

As mentioned earlier, for each byte, we test it 100 times.  
We collect all results that **successfully detect 8 accesses to *memory*.** in final_result.txt  
When the waiting window, it would be hard to collect such results.  
On i5-8265U with governor setting to be performance, we get on average 15 such samples during each run.


