import json
from matplotlib import pyplot as plt
from matplotlib import patches as patches

file_path = "build/accelerate_effectiveness.json"
figure_path = "figures/accelerate_effectiveness.svg"
model_path = "../data/opt-350m-16.json"
model_name = "OPT-350M"
min_bandwidth = 5e8
max_bandwidth = 10e9

with open(file_path, "r") as f:
    data = json.load(f)

series = data[model_path]
bandwidth = [x[0]/1e9 for x in series if min_bandwidth <= x[0] <= max_bandwidth]
jct = [x[1] for x in series if min_bandwidth <= x[0] <= max_bandwidth]

plt.rcParams["font.family"] = "Times New Roman"
fig, ax = plt.subplots(figsize=(4,2))
fig.plot(bandwidth, jct, label=model_name, color=(40/255,120/255,181/255))
fig.xlabel("Aggregation bandwidth (GB/s)")
fig.ylabel("Iteration duration (s)")
fig.legend()

ax.add_patch(patches.Rectangle((1, 1), 2, 3, linewidth=1, edgecolor='r', facecolor='blue', alpha=0.5))

plt.tight_layout(pad=0.5)
plt.savefig(figure_path, dpi=400)
