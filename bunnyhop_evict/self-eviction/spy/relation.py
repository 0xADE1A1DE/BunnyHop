from scipy import stats
import numpy as np
import matplotlib.pyplot as plt


cache_line0 = [0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76]
def access_table(a):
  if a in cache_line0:
    return 1
  else:
    return 0

lines = []
count = 0
#limit_count = 100000 #00 
limit_count = 35500 #35500

with open('result_0xff.txt') as f:
  lines = f.readlines()

count2=0
cts = [0] * (limit_count+1) 
times = [0] * (limit_count+1)
for line in lines:
  s = line.split()
  cts[count2] = int(s[0][0:2],16)
  times[count2] = int(s[1])
  count2 += 1
  if count2 > limit_count:
    break


tmp_count = 0
result = [[0 for x in range(limit_count+1)] for y in range(256)]
guess = 0
for ct in cts:
  for guess in range(256):
    result[guess][tmp_count] = access_table(ct ^ guess)
  tmp_count += 1


experiment_num = 256 
stride = 500 #1000
stride2 = 500 #1000
plot_limit = int(limit_count / stride)
print(plot_limit, int(plot_limit))
#plot_me = [[0 for x in range(limit_count+1)] for y in range(256)]
plot_me = [[0 for x in range(plot_limit+1)] for y in range(experiment_num)]
tmp_count = 0
for guess in range(experiment_num):
  tmp_count = 0
  for i in range(plot_limit):
    if i == 0:
      r = 1
    else:
      r = stats.pearsonr(times[0: i*stride2], result[guess][0: i*stride2])[0]
    plot_me[guess][tmp_count] = r
    tmp_count += 1
  print(guess)

x_axis = np.arange(plot_limit+1)
plt.figure(figsize=(5,2.5))
plt.rcParams['pdf.fonttype'] = 42
plt.rcParams['ps.fonttype'] = 42
for guess in range(experiment_num):
  if guess != 48:
    plt.plot(x_axis, plot_me[guess], alpha=0.5, linewidth=1)
plt.plot(x_axis, plot_me[48], color = "blue", linewidth=1, label="Correct Guess: 0x30")

x_axis2 = np.arange(0, plot_limit+1, 20)
plt.ylim((-0.05, 0.15))
plt.xlim((0,int(plot_limit/10)))
plt.xticks(np.arange(0,plot_limit,10), np.arange(0,limit_count,5000))

plt.xlabel("Number of Ciphertexts")
plt.ylabel("Pearson Correlation")
plt.legend()
plt.tight_layout()
plt.savefig("result.pdf")
