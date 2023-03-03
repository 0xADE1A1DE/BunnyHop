import numpy as np
import matplotlib
import matplotlib.pyplot as plt
#import seaborn as sns
import sys
import re

samples = 100
num = 45
data_long = [0] * 32
data_short = [0] * 32
data_short2 = [0] * 32

count_long = [0] * 32
count_short = [0] * 32


f = "result.txt"
with open(f) as file:
    lines = file.readlines()
    for line in lines:
        data = re.findall(r'\d+', line)
        if len(data) != 0:
            if int(data[0]) == 0:
                data_long[int(data[1]) - 14] += int(data[2])
                count_long[int(data[1]) - 14]  += 1
            else:
                data_short[int(data[1]) - 14] += int(data[2])
                data_short2[int(data[1]) - 14] += int(data[3])
                count_short[int(data[1]) - 14] += 1

data_short2 = np.add(data_short2, data_short)

x_axis = np.arange(14,46,1)
plt.plot(x_axis, data_long,label="long branch")
plt.plot(x_axis, data_short,label="short branch")
plt.plot(x_axis, data_short2,label="combined")

plt.legend()
plt.ylabel("Success rate")
plt.xlabel("K")
plt.xticks(x_axis)
plt.locator_params(axis='x', nbins=9)
plt.savefig("result.pdf")
