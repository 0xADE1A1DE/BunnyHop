import numpy as np
import matplotlib.pyplot as plt

## [byte]
time_result = [0] * 256;
number_count = [0] * 256;

lines = []
count = 0
with open('result_0xff.txt') as f:
	lines = f.readlines()

for line in lines:
	s = line.split()
	time_result[int(s[0][0:2],16)] += int(s[1])
	number_count[int(s[0][0:2],16)] += 1


final_result = [0] * 256
cc = 0
for (c,s) in zip(number_count, time_result):
	final_result[cc] = s/c
	if (final_result[cc] > 5440):
		print(cc, final_result[cc])
	cc += 1


x1 = [0] * 256
for i in range(0,256):
	x1[i] = i

x = np.arange(0,256)
plt.plot(x1,final_result)
plt.ylabel("Timing Measurement(cycles)")
plt.xlabel("The first byte of ciphertexts")
plt.savefig("graph_0xff.pdf")


#print(count)

#for line in 
#s = f.readline().split()
#print(s[0], s[1])
