import json
import numpy as np
from matplotlib import pyplot as plt

file_path = "results/job_placement.json"
figure_path = "figures/job-placement.pdf"

with open(file_path, "r") as f:
    data = json.load(f)
result_mina = data["mina"]
result_baseline = data["baseline"]

jct_score_mina = np.array([result_mina[i*8+i]["JCTScoreWeighted"] for i in range(8)])
jct_score_baseline = np.array([result_baseline[i*8+i]["JCTScoreWeighted"] for i in range(8)])
sharp_ratio_mina = np.array([result_mina[i*8+i]["SharpRatioWeighted"] for i in range(8)])
sharp_ratio_baseline = np.array([result_baseline[i*8+i]["SharpRatioWeighted"] for i in range(8)])

x_range = np.arange(8)
bar_width = 0.35

plt.rcParams["font.family"] = ["Times New Roman", "SimSong"]
plt.rcParams["font.size"] = 12
plt.rcParams["pdf.fonttype"] = 42
plt.rcParams["ps.fonttype"] = 42
fig, axs = plt.subplots(1, 2, figsize=(8, 3))
axs[0].grid(axis="y", zorder=0)
axs[0].bar(x_range, jct_score_baseline, bar_width, label="基准方法", color=(200/255,36/255,35/255), zorder=2)
axs[0].bar(x_range+bar_width, jct_score_mina, bar_width, label="MINA", color=(40/255,120/255,181/255), zorder=2)
axs[0].legend()
axs[0].set_xlabel("链路超额比例")
axs[0].set_ylabel("在网聚合效能评分")
axs[0].set_xticks(x_range+bar_width/2, [f"8:{i+1}" for i in range(8)])
axs[1].grid(axis="y", zorder=0)
axs[1].bar(x_range, sharp_ratio_baseline, bar_width, label="基准方法", color=(200/255,36/255,35/255), zorder=2)
axs[1].bar(x_range+bar_width, sharp_ratio_mina, bar_width, label="MINA", color=(40/255,120/255,181/255), zorder=2)
axs[1].legend()
axs[1].set_xlabel("链路超额比例")
axs[1].set_ylabel("在网聚合利用率")
axs[1].set_xticks(x_range+bar_width/2, [f"8:{i+1}" for i in range(8)])
plt.tight_layout(pad=0.5)
plt.savefig(figure_path, dpi=400)

print(jct_score_mina.max())
print(sharp_ratio_mina.max())
print(jct_score_baseline.mean())
print(sharp_ratio_baseline.mean())
