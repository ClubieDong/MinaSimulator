import json
import numpy as np
from matplotlib import pyplot as plt

file_path = "results/large_scale_simulation.json"
figure_path = "figures/overall_performance.pdf"

with open(file_path, "r") as f:
    data = json.load(f)

mina_jct_score = np.array([data[i*2]["JCTScoreWeighted"] for i in range(10)])
baseline_jct_score = np.array([data[i*2+1]["JCTScoreWeighted"] for i in range(10)])
mina_sharp_ratio = np.array([data[i*2]["SharpRatioWeighted"] for i in range(10)])
baseline_sharp_ratio = np.array([data[i*2+1]["SharpRatioWeighted"] for i in range(10)])

index = np.arange(10)
bar_width = 0.35

plt.rcParams["font.family"] = ["Times New Roman", "SimSun"]
plt.rcParams["font.size"] = 12
plt.rcParams["pdf.fonttype"] = 42
plt.rcParams["ps.fonttype"] = 42
fig, axs = plt.subplots(1, 2, figsize=(8, 3))
axs[0].grid(axis="y", zorder=0)
axs[0].bar(index, baseline_jct_score, bar_width, label="基准方法", color=(200/255,36/255,35/255), zorder=2)
axs[0].bar(index + bar_width, mina_jct_score, bar_width, label="MINA", color=(40/255,120/255,181/255), zorder=2)
axs[0].legend(loc="upper left")
axs[0].set_xlabel("采集数据编号")
axs[0].set_ylabel("在网聚合效能评分")
axs[0].set_xticks(index + bar_width / 2, [f"{i+1}" for i in range(10)])
axs[0].yaxis.set_major_formatter("{x:.2f}")  

axs[1].grid(axis="y", zorder=0)
axs[1].bar(index, baseline_sharp_ratio, bar_width, label="基准方法", color=(200/255,36/255,35/255), zorder=2)
axs[1].bar(index + bar_width, mina_sharp_ratio, bar_width, label="MINA", color=(40/255,120/255,181/255), zorder=2)
axs[1].legend(loc="upper left")
axs[1].set_xlabel("采集数据编号")
axs[1].set_ylabel("在网聚合利用率")
axs[1].set_xticks(index + bar_width / 2, [f"{i+1}" for i in range(10)])
axs[1].yaxis.set_major_formatter("{x:.2f}")  

fig.tight_layout(pad=0.5)
fig.savefig(figure_path, dpi=400)

print("mina_jct_score:", mina_jct_score.mean(), mina_jct_score.max())
print("baseline_jct_score:", baseline_jct_score.mean(), baseline_jct_score.max())
print("mina_jct_score/baseline_jct_score:", (mina_jct_score/baseline_jct_score).mean(), (mina_jct_score/baseline_jct_score).max())
print()
print("mina_sharp_ratio:", mina_sharp_ratio.mean(), mina_sharp_ratio.max())
print("baseline_sharp_ratio:", baseline_sharp_ratio.mean(), baseline_sharp_ratio.max())
print("mina_sharp_ratio/baseline_sharp_ratio:", (mina_sharp_ratio/baseline_sharp_ratio).mean(), (mina_sharp_ratio/baseline_sharp_ratio).max())
