import json
import math
import numpy as np
from matplotlib import pyplot as plt
from matplotlib.colors import LinearSegmentedColormap

file_path = "results/sharing.json"
figure1_path = "figures/sharing_matrix.pdf"
figure2_path = "figures/sharing_hist.pdf"

model_name = {
    "traces/opt-125m-4.json": "OPT-125M",
    "traces/opt-350m-4.json": "OPT-350M",
    "traces/opt-1.3b-4.json": "OPT-1.3B",
    "traces/bert-base-4.json": "BERT-base",
    "traces/bert-large-4.json": "BERT-large",
    "traces/vit-base-4.json": "ViT-base",
    "traces/vit-large-4.json": "ViT-large",
}

with open(file_path, "r") as f:
    data = json.load(f)
model_list = data["model_list"]
model_list = [model_name[x] for x in model_list]
res_2_jobs = data["result_2_jobs"]
res_3_jobs = data["result_3_jobs"]
res_4_jobs = data["result_4_jobs"]
print(f"2 jobs, mean={np.mean(res_2_jobs)}, max={np.max(res_2_jobs)}")
print(f"2 jobs ==1.0: {sum(1 for x in res_2_jobs if x >= 0.995)}")
print(f"2 jobs >=0.9: {sum(1 for x in res_2_jobs if x >= 0.9)}")
print(f"2 jobs >=0.95: {sum(1 for x in res_2_jobs if x >= 0.95)}")

mat_2_jobs = [[res_2_jobs[i * len(model_list) + j] for j in range(len(model_list))] for i in range(len(model_list))]

plt.rcParams["font.family"] = ["Times New Roman", "SimSun"]
plt.rcParams["font.size"] = 12
plt.rcParams["pdf.fonttype"] = 42
plt.rcParams["ps.fonttype"] = 42

cm = LinearSegmentedColormap.from_list("my_colormap", [(255/255, 217/255, 102/255), (255/255, 242/255, 204/255), (169/255, 209/255, 142/255)], N=1000)
plt.figure(figsize=(5, 4))
plt.imshow(mat_2_jobs, cmap=cm)
plt.xticks(range(len(model_list)), model_list, rotation=45)
plt.yticks(range(len(model_list)), model_list)
plt.colorbar()
for i in range(len(mat_2_jobs)):
    for j in range(len(mat_2_jobs[i])):
        plt.text(j, i, f"{mat_2_jobs[i][j]:.2f}", ha="center", va="center", color="k")
plt.tight_layout(pad=0.5)
plt.savefig(figure1_path, dpi=400)
plt.close()

bins = 30
fig, axs = plt.subplots(1, 2, figsize=(8, 3.5))

pdf_counts, bin_edges = np.histogram(res_3_jobs, bins=bins, density=True)
bin_widths = np.diff(bin_edges)
cdf = np.cumsum(pdf_counts * bin_widths)
cdf = np.insert(cdf, 0, 0)
mean, median = np.mean(res_3_jobs), np.median(res_3_jobs)
ax1 = axs[0]
ax2 = ax1.twinx()
bars = ax1.bar(bin_edges[:-1], pdf_counts, width=bin_widths, align="edge", alpha=1.0, color=(40/255,120/255,181/255), label="概率分布", zorder=2)
line, = ax2.step(bin_edges, cdf, where="post", color=(200/255,36/255,35/255), linewidth=2, label="累积分布", zorder=2)
ax1.set_xlabel("在网聚合效能评分")
ax1.set_ylabel("概率分布")
ax2.set_ylabel("累积分布")
ax1.set_title("3个任务共享", fontsize=12)
ax1.grid(axis="y", zorder=0)
ax1.set_xlim(0.4, 1)
vl1 = ax1.axvline(x=mean, color="red", linestyle="--", linewidth=1, label="平均值")
vl2 = ax1.axvline(x=median, color="blue", linestyle="--", linewidth=1, label="中位值")
lines = [bars, line, vl1, vl2]
labels = [l.get_label() for l in lines]
ax1.legend(lines, labels, loc="upper left")
print("3 jobs", mean, median)

pdf_counts, bin_edges = np.histogram(res_4_jobs, bins=bins, density=True)
bin_widths = np.diff(bin_edges)
cdf = np.cumsum(pdf_counts * bin_widths)
cdf = np.insert(cdf, 0, 0)
mean, median = np.mean(res_4_jobs), np.median(res_4_jobs)
ax1 = axs[1]
ax2 = ax1.twinx()
bars = ax1.bar(bin_edges[:-1], pdf_counts, width=bin_widths, align="edge", alpha=1.0, color=(40/255,120/255,181/255), label="概率分布", zorder=2)
line, = ax2.step(bin_edges, cdf, where="post", color=(200/255,36/255,35/255), linewidth=2, label="累积分布", zorder=2)
ax1.set_xlabel("在网聚合效能评分")
ax1.set_ylabel("概率分布")
ax2.set_ylabel("累积分布")
ax1.set_title("4个任务共享", fontsize=12)
ax1.grid(axis="y", zorder=0)
ax1.set_xlim(0.4, 1)
vl1 = ax1.axvline(x=mean, color="red", linestyle="--", linewidth=1, label="平均值")
vl2 = ax1.axvline(x=median, color="blue", linestyle="--", linewidth=1, label="中位值")
lines = [bars, line, vl1, vl2]
labels = [l.get_label() for l in lines]
ax1.legend(lines, labels, loc="upper left")
print("4 jobs", mean, median)

plt.tight_layout()
plt.savefig(figure2_path, dpi=400)
plt.close()
