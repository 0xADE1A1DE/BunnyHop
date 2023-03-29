The code under this folder provides code for BunnyHop-evict attack on AES kernel module.
**The cache coloring we provide only targets at Intel processors with 4 physical cores.**

- Compile and install the kernel
The colored kernel source code is available at: https://doi.org/10.5281/zenodo.7704477
We provied the scipt to build kernel (`compile_kernel.bash`)
After the compilation, install the kernel with command `$sudo dpkg -i *.deb`
In the end update the grub and restart the machine `sudo update-grub && sudo reboot`  

Or you could follow the methods at https://phoenixnap.com/kb/build-linux-kernel or any methods you like.  

- Compile and install color_handler module.
`$cd color_module/`  
'$make'  
'$sudo insmod color_handler.ko'
`$dmesg` to check if it is properly installed

- Compile and install AES module
'$bash load.sh'  
Regist the device with command:  
`$sudo mknod /dev/cdc c MAJOR 0`  
Note that you should get a device major number in the output of the last step.  

- Update test.bash 
Now you need to update the *BASE* with the function address obtained in the previous output.  

- Run test
Now, simply run command `bash test.bash` will start doing bunnyhop-evict attack on AES.

- Plot Graph
Wait for several seconds and the attack will be completed.  
To plot the peasrson relation graph (Figure 5), you need to execute command `python3 relation.py`.  
It takes a while to plot the graph.  

Alternatively, you can run `python3 process.py` to plot 16 peaks.

## Results
We provide the expecte result in pearson.pdf and sbox_access.pdf.

## A video guide
We also provide a video guide on evaluating the self-eviction: https://youtu.be/2nW5m3-qStw
