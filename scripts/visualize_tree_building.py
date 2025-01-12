import json
from matplotlib import pyplot as plt

file_path = "results/tree_building.json"
figure_path = "figures/tree-building.pdf"

with open(file_path, "r") as f:
    data = json.load(f)

tree_building = [x["TimeCostTreeBuilding"] for x in data]
jct_score = [x["JCTScore"] for x in data]
max_tree_count = [x["MaxTreeCount"] for x in data]
x_range = list(range(len(max_tree_count)))
x_range[-1] += 0.5

plt.rcParams["font.family"] = "Times New Roman"
plt.rcParams["pdf.fonttype"] = 42
plt.rcParams["ps.fonttype"] = 42
fig, ax1 = plt.subplots(figsize=(4, 2.5))
ax1.grid(axis="y", zorder=0)
ax1.bar(x_range, tree_building, 0.6, label="Running time", color=(40/255,120/255,181/255), zorder=2)
ax1.set_ylim(0, 100)
ax1.set_xlabel("# of candidate aggregation trees")
ax1.set_ylabel("Running time (ms)")
ax1.set_xticks(x_range, max_tree_count)
ax1.text(x_range[-1], 101, f"{tree_building[-1]:.1f} ms", ha="center", va="bottom", color="k")

ax2 = ax1.twinx()
ax2.plot(x_range, jct_score, label="Performance", color=(200/255,36/255,35/255), marker="o", zorder=2)
ax2.set_ylim(0.82, 0.94)
ax2.set_ylabel("INA efficiency score")

handles1, labels1 = ax1.get_legend_handles_labels()
handles2, labels2 = ax2.get_legend_handles_labels()
handles = handles1 + handles2
labels = labels1 + labels2
ax1.legend(handles, labels)

plt.tight_layout(pad=0.5)
plt.savefig(figure_path, dpi=400)
