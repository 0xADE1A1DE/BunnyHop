import numpy as np
import matplotlib
import matplotlib.pyplot as plt
#import seaborn as sns
import sys
import re

overall_bits = 0
with open ("overall_result.txt") as file:
    lines = file.readlines()
    lines = [line.rstrip() for line in lines]
    for line in lines:
        #print(line.split(" "))
        #array_one = re.findall(r'\d+', line)
        #print(array_one)
        overall_bits += int(line.split(" ")[8])
print(overall_bits / 800)