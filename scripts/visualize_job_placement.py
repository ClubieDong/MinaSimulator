import json
import numpy as np
from matplotlib import pyplot as plt

file_path = "results/job_placement.json"
figure_path = "figures/job-placement.pdf"

with open(file_path, "r") as f:
    data = json.load(f)

data1 = np.array([x[0] for x in data])
data2 = np.array([x[1] for x in data])
data3 = np.array([x[2] for x in data])
x_range = np.arange(len(data))

bar_width = 0.35
plt.rcParams["font.family"] = "Times New Roman"
plt.rcParams["pdf.fonttype"] = 42
plt.rcParams["ps.fonttype"] = 42
plt.figure(figsize=(4, 2.5))
plt.grid(axis="y", zorder=0)
# plt.bar(x_range, data1, bar_width, label="Random", color=(200/255,36/255,35/255), zorder=2)
plt.bar(x_range, data2, bar_width, label="Baseline", color=(200/255,36/255,35/255), zorder=2)
plt.bar(x_range+bar_width, data3, bar_width, label="MINA", color=(40/255,120/255,181/255), zorder=2)
plt.legend()
plt.xlabel("Oversubscription ratio")
plt.ylabel("INA efficiency score")
plt.xticks(x_range+bar_width/2, [f"8:{i+1}" for i in range(8)])
plt.tight_layout(pad=0.5)
plt.savefig(figure_path, dpi=400)

print((data3-data2).mean(), (data3-data2).max())
print(data3)
