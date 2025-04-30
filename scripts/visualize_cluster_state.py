import json
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap

plt.rcParams["font.family"] = ["Times New Roman", "SimSun"]
plt.rcParams["font.size"] = 12
plt.rcParams["pdf.fonttype"] = 42
plt.rcParams["ps.fonttype"] = 42
fig, axs = plt.subplots(1, 2, figsize=(10, 1.8))

with open("results/cluster_state_baseline.json", "r") as f:
    data = json.load(f)
state = [0 for _ in range(1024)]
for i, hosts in enumerate(data):
    for host in hosts:
        state[host] = i + 1
state = np.array(state).reshape(16, 64)
cmap = plt.get_cmap("gist_rainbow", len(data) + 1)
colors = cmap(np.arange(len(data) + 1))
colors[0] = (0.0, 0.0, 0.0, 1.0)
new_cmap = ListedColormap(colors)
axs[0].matshow(state, cmap=new_cmap)
axs[0].set_xticks([])
axs[0].set_yticks([])
axs[0].set_title("基准方法")

with open("results/cluster_state_mina.json", "r") as f:
    data = json.load(f)
state = [0 for _ in range(1024)]
for i, hosts in enumerate(data):
    for host in hosts:
        state[host] = i + 1
state = np.array(state).reshape(16, 64)
cmap = plt.get_cmap("gist_rainbow", len(data) + 1)
colors = cmap(np.arange(len(data) + 1))
colors[0] = (0.0, 0.0, 0.0, 1.0)
new_cmap = ListedColormap(colors)
axs[1].matshow(state, cmap=new_cmap)
axs[1].set_xticks([])
axs[1].set_yticks([])
axs[1].set_title("MINA")

plt.tight_layout(pad=0.5)
plt.subplots_adjust(top=0.85, bottom=0.05) 
plt.savefig("figures/cluster_state.pdf", dpi=400)
