import json
import numpy as np
from matplotlib import pyplot as plt

file_path = "results/traces.json"
figure_path = "figures/traces.pdf"

with open(file_path, "r") as f:
    traces = json.load(f)

min_arr, max_arr, mean_arr, std_arr = [], [], [], []
for trace in traces:
    hosts = []
    for host_count, repeat in trace:
        hosts.extend([host_count] * repeat)
    hosts = np.array(hosts)
    min_arr.append(hosts.min())
    max_arr.append(hosts.max())
    mean_arr.append(hosts.mean())
    std_arr.append(hosts.std())

print("min_arr:", min_arr)
print("max_arr:", max_arr)
print("mean_arr:", mean_arr)
print("std_arr:", std_arr)

plt.rcParams["font.family"] = ["Times New Roman", "SimSun"]
plt.rcParams["font.size"] = 12
plt.rcParams["pdf.fonttype"] = 42
plt.rcParams["ps.fonttype"] = 42
plt.subplots(figsize=(5, 2))
plt.errorbar(
    x=[i + 1 for i in range(len(traces))],
    y=mean_arr,
    yerr=std_arr,
    fmt="o",
    color=(40/255,120/255,181/255),
    capsize=3,
)
plt.xlabel("Trace ID")
plt.ylabel("Average Job Size")
plt.xticks([i + 1 for i in range(len(traces))])
plt.tight_layout(pad=0.5)
plt.savefig(figure_path, dpi=400)
