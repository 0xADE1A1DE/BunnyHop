import numpy as np
import matplotlib
import matplotlib.pyplot as plt
#import seaborn as sns
import sys
import re

probe_time = 1024
fr_time = [0] * probe_time
pp_result = [0] * probe_time

do_one = 0
do_two = 0
num_samples = 1
results = [0] * 8
samples = 0

with open ("final_result.txt") as file:
    lines = file.readlines()
    lines = [line.rstrip() for line in lines]
    samples = len(lines)
    for line in lines:
        array_one = re.findall(r'\d+', line)
        local_count = 0
        for v in array_one:
            results[local_count] = results[local_count] + int(v)
            local_count += 1
file.close()

secret = 0
local_count = 0
for v in results:
    print('%.2f' % (v/samples), end=" ")
    if v > (0.4 * samples):
        secret |= (1 << local_count)
    local_count += 1
#print()
#print(secret, int(sys.argv[1]), secret == int(sys.argv[1]))

guess_count = 0
for i in range(0,8):
    g_bit = (secret >> i) & 1
    s_bit = (int(sys.argv[1]) >> i) & 1
    if g_bit == s_bit:
        guess_count += 1
print(guess_count, secret, int(sys.argv[1]))