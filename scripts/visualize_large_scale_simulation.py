import json
import numpy as np
from matplotlib import pyplot as plt

file_path = "results/large_scale_simulation.json"
figure_path = "figures/overall-performance.pdf"

with open(file_path, "r") as f:
    data = json.load(f)

mina_jct_score = np.array([data[i*2]["JCTScore"] for i in range(10)])
baseline_jct_score = np.array([data[i*2+1]["JCTScore"] for i in range(10)])
mina_sharp_ratio = np.array([data[i*2]["SharpRatio"] for i in range(10)])
baseline_sharp_ratio = np.array([data[i*2+1]["SharpRatio"] for i in range(10)])

index = np.arange(10)
bar_width = 0.35

plt.rcParams["font.family"] = "Times New Roman"
fig, axs = plt.subplots(1, 2, figsize=(7, 2.5))
axs[0].grid(axis="y", zorder=0)
axs[0].bar(index, baseline_jct_score, bar_width, label="Baseline", color=(200/255,36/255,35/255), zorder=2)
axs[0].bar(index + bar_width, mina_jct_score, bar_width, label="MINA", color=(40/255,120/255,181/255), zorder=2)
axs[0].legend(loc="upper left")
axs[0].set_xlabel("Trace ID")
axs[0].set_ylabel("INA efficiency score")
axs[0].set_xticks(index + bar_width / 2, [f"{i+1}" for i in range(10)])

axs[1].grid(axis="y", zorder=0)
axs[1].bar(index, baseline_sharp_ratio, bar_width, label="Baseline", color=(200/255,36/255,35/255), zorder=2)
axs[1].bar(index + bar_width, mina_sharp_ratio, bar_width, label="MINA", color=(40/255,120/255,181/255), zorder=2)
axs[1].legend(loc="upper left")
axs[1].set_xlabel("Trace ID")
axs[1].set_ylabel("INA utilization rate")
axs[1].set_xticks(index + bar_width / 2, [f"{i+1}" for i in range(10)])

fig.tight_layout(pad=0.5)
fig.savefig(figure_path, dpi=400)

print("baseline_jct_score:", baseline_jct_score.mean(), baseline_jct_score.max(), baseline_jct_score.min())
print("mina_jct_score:", mina_jct_score.mean(), mina_jct_score.max(), mina_jct_score.min())
print("baseline_sharp_ratio:", baseline_sharp_ratio.mean(), baseline_sharp_ratio.max(), baseline_sharp_ratio.min())
print("mina_sharp_ratio:", mina_sharp_ratio.mean(), mina_sharp_ratio.max(), mina_sharp_ratio.min())
print("mina_sharp_ratio/baseline_sharp_ratio", (mina_sharp_ratio/baseline_sharp_ratio).mean(), (mina_sharp_ratio/baseline_sharp_ratio).max())
