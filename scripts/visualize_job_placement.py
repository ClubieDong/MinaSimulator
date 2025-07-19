import json
import numpy as np
from matplotlib import pyplot as plt

file_path = "results/job_placement.json"
figure_path = "figures/job_placement.pdf"

with open(file_path, "r") as f:
    data = json.load(f)
result_mina = data["mina"]
result_baseline = data["baseline"]

jct_score_mina = np.array([result_mina[i]["JCTScoreWeighted"] for i in range(8)])
jct_score_baseline = np.array([result_baseline[i]["JCTScoreWeighted"] for i in range(8)])
sharp_ratio_mina = np.array([result_mina[i]["SharpRatioWeighted"] for i in range(8)])
sharp_ratio_baseline = np.array([result_baseline[i]["SharpRatioWeighted"] for i in range(8)])

x_range = np.arange(8)
bar_width = 0.35

plt.rcParams["font.family"] = ["Times New Roman", "SimSun"]
plt.rcParams["font.size"] = 12
plt.rcParams["pdf.fonttype"] = 42
plt.rcParams["ps.fonttype"] = 42
fig, axs = plt.subplots(1, 2, figsize=(8, 3))
axs[0].grid(axis="y", zorder=0)
axs[0].bar(x_range, jct_score_baseline, bar_width, label="Baseline", color=(200/255,36/255,35/255), zorder=2)
axs[0].bar(x_range+bar_width, jct_score_mina, bar_width, label="MINA", color=(40/255,120/255,181/255), zorder=2)
axs[0].legend()
axs[0].set_xlabel("Oversubscription ratio")
axs[0].set_ylabel("INA efficiency score")
axs[0].set_xticks(x_range+bar_width/2, [f"8:{i+1}" for i in range(8)])
axs[1].grid(axis="y", zorder=0)
axs[1].bar(x_range, sharp_ratio_baseline, bar_width, label="Baseline", color=(200/255,36/255,35/255), zorder=2)
axs[1].bar(x_range+bar_width, sharp_ratio_mina, bar_width, label="MINA", color=(40/255,120/255,181/255), zorder=2)
axs[1].legend()
axs[1].set_xlabel("Oversubscription ratio")
axs[1].set_ylabel("INA utilization rate")
axs[1].set_xticks(x_range+bar_width/2, [f"8:{i+1}" for i in range(8)])
plt.tight_layout(pad=0.5)
plt.savefig(figure_path, dpi=400)

print("JCTScore max:", jct_score_mina.max())
print("SharpRatio max:", sharp_ratio_mina.max())
print("JCTScore mean:", jct_score_baseline.mean())
print("SharpRatio mean:", sharp_ratio_baseline.mean())
