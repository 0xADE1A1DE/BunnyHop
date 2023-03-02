import sys
import matplotlib.pyplot as plt
import numpy as np
import re

threshold = 50
data_idle = [0] * 51
data_busy = [0] * 51

for i in range(0,51):
	file_idle = open("results/idle/nop"+str(i)+".txt")
	file_busy = open("results/busy/nop"+str(i)+".txt")
	lines_idle = file_idle.readlines()
	lines_busy = file_busy.readlines()
	tmp_hit_dile = 0
	tmp_hit_busy = 0
	for tmp in range(0,21):
		val_idle = re.findall(r'\d+', lines_idle[tmp])
		val_busy = re.findall(r'\d+', lines_busy[tmp])
		if int(val_idle[0]) < threshold:
			tmp_hit_idle = tmp
		if int(val_busy[0]) < threshold:
			tmp_hit_busy = tmp
	data_idle[i] = tmp_hit_idle
	data_busy[i] = tmp_hit_busy
	file_idle.close()
	file_busy.close()
print(data_idle)
print(data_busy)


plt.figure(figsize=(6, 3), dpi=80)
xaxis = np.arange(51)
plt.plot(xaxis, data_idle, label="Idle", linewidth=2, drawstyle='steps-post')
plt.plot(xaxis, data_busy, label="Busy", linewidth=2, drawstyle='steps-post', linestyle='--')

plt.legend()
plt.xlabel('Number of NOP instructions')
plt.ylabel('Prefetch depth')
plt.savefig("result.pdf", bbox_inches='tight')