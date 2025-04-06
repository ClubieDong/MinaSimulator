import json
from matplotlib import pyplot as plt

file_path = "results/tree_building.json"
figure_path = "figures/tree-building.pdf"

with open(file_path, "r") as f:
    data = json.load(f)

tree_building = [x["TimeCostTreeBuilding"] / x["FinishedJobCount"] for x in data]
jct_score = [x["JCTScoreWeighted"] for x in data]
max_tree_count = list(range(1, len(tree_building) + 1))
max_tree_count[-1] = "全部"
x_range = list(range(len(max_tree_count)))
x_range[-1] += 0.5

plt.rcParams["font.family"] = ["Times New Roman", "SimSong"]
plt.rcParams["font.size"] = 12
plt.rcParams["pdf.fonttype"] = 42
plt.rcParams["ps.fonttype"] = 42
fig, ax1 = plt.subplots(figsize=(5, 3))
ax1.grid(axis="y", zorder=0)
ax1.bar(x_range, tree_building, 0.6, label="算法运行时间", color=(40/255,120/255,181/255), zorder=2)
ax1.set_ylim(0, 330)
ax1.set_xlabel("候选聚合树数量")
ax1.set_ylabel("运行时间 (毫秒)")
ax1.set_xticks(x_range, max_tree_count)
ax1.text(x_range[-1], 330, f"{tree_building[-1]:.1f} ms", ha="center", va="bottom", color="k")

ax2 = ax1.twinx()
ax2.plot(x_range, jct_score, label="算法性能表现", color=(200/255,36/255,35/255), marker="o", zorder=2)
ax2.set_ylabel("在网聚合效能评分")

handles1, labels1 = ax1.get_legend_handles_labels()
handles2, labels2 = ax2.get_legend_handles_labels()
handles = handles1 + handles2
labels = labels1 + labels2
ax1.legend(handles, labels)

plt.tight_layout(pad=0.5)
plt.savefig(figure_path, dpi=400)
