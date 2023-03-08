The code under this folder is a PoC on breaking KASLR with BunnyHop-Reload.


## How to start.
1. Firstly, you will need to find the start address of `kill_something_info` with command below:  
`$sudo cat /boot/System.map-$(uname -r) | grep kill_something_info`  
The obtained address is not randomized.

2. Secondly, we will need to find the default address of the vulnerable branch.  
To do it, you firstly need to dump the kernel image.  You can follow the link  
https://blog.packagecloud.io/how-to-extract-and-disassmble-a-linux-kernel-image-vmlinuz/  
to get the basic physical address for the vulnerable address.

The branch we target is a call instruction. We highlight the vulnerable branch in the branch_location.png. 


3. Thirdly, you need to change the value of `branch_adrs` with the 32 LSBs of the vulnerable branch address. Then you need to change the value of `branch_target` with the 32 LSBs of the branch target address.  

4. Now, you can compile the program with command `$make`.

5. You can run `bash test.bash` to conduct the attack and the correct result 

6. You can verify the result with command sudo `cat /proc/kallsyms | grep kill_something_info`  
**Note that the program returns the physical address of the tatget branch. So the lower bits of the returned address and the *kill_something_info* will differ in lower bits.**
