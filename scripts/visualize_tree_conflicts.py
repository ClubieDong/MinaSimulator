import json
import math
import numpy as np
from tqdm import trange
from matplotlib import pyplot as plt

file_path = "results/tree_conflict_trace.json"
figure_path = "figures/conflicts-vs-fragments.pdf"
sliding_window_size = 1000

with open(file_path, "r") as f:
    data = json.load(f)

x_range = np.arange(sliding_window_size//2-1, len(data["tree_conflicts"])-sliding_window_size//2, dtype=np.int_)

allocated_frags, available_frags = [], []
for idx in trange(len(data["tree_conflicts"])):
    points = [(allocated, available) for n_job, (allocated, available) in data["host_fragments"] if n_job-sliding_window_size/2 <= idx <= n_job+sliding_window_size/2]
    if len(points) == 0:
        allocated_frags.append(math.nan)
        available_frags.append(math.nan)
    else:
        allocated_frags.append(sum(x for x, _ in points) / len(points))
        available_frags.append(sum(x for _, x in points) / len(points))
allocated_frags = np.array(allocated_frags)[x_range]
available_frags = np.array(available_frags)[x_range]
total_frags = allocated_frags + available_frags

tree_conflicts = np.array(data["tree_conflicts"])
tree_conflicts = np.convolve(tree_conflicts, np.ones(sliding_window_size)/sliding_window_size, mode="valid")

plt.rcParams["font.family"] = "Times New Roman"
fig, ax1 = plt.subplots(figsize=(4, 2))
ax2 = ax1.twinx()
ax1.grid(zorder=0)
ax1.plot(x_range, tree_conflicts, color=(200/255,36/255,35/255), label="Probability of tree conflict", zorder=2)
ax2.plot(x_range, total_frags, color=(40/255,120/255,181/255), label="# of host fragments", zorder=2)
ax1.set_xlabel("# of processed requests")
ax1.set_ylabel("Probability")
ax2.set_ylabel("# of fragments")
handles1, labels1 = ax1.get_legend_handles_labels()
handles2, labels2 = ax2.get_legend_handles_labels()
handles = handles1 + handles2
labels = labels1 + labels2
ax1.legend(handles, labels, loc="lower right")
plt.tight_layout(pad=0.5)
plt.savefig(figure_path, dpi=400)
