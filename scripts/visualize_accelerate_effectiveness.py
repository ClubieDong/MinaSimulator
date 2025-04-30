import json
from matplotlib import pyplot as plt
from matplotlib import patches as patches

file_path = "results/accelerate_effectiveness.json"
figure_path = "figures/accelerate_effectiveness.svg"
model_path = "traces/opt-350m-16.json"
model_name = "OPT-350M"
min_bandwidth = 5e8
max_bandwidth = 10e9

with open(file_path, "r") as f:
    data = json.load(f)

series = data[model_path]
bandwidth = [x[0]/1e9 for x in series if min_bandwidth <= x[0] <= max_bandwidth]
jct = [x[1] for x in series if min_bandwidth <= x[0] <= max_bandwidth]

plt.rcParams["font.family"] = ["Times New Roman", "SimSun"]
plt.rcParams["font.size"] = 12
plt.rcParams["pdf.fonttype"] = 42
plt.rcParams["ps.fonttype"] = 42
fig, ax = plt.subplots(figsize=(6,3))
ax.plot(bandwidth, jct, label=model_name, color=(40/255,120/255,181/255), zorder=2)
ax.set_xlabel("通信带宽 (GB/s)")
ax.set_ylabel("训练批次时长 (s)")
ax.legend()

plt.tight_layout(pad=0.5)
plt.savefig(figure_path, dpi=400)
