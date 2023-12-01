import json
import math
from matplotlib import pyplot as plt

file_path = "build/accelerate_effectiveness.json"
figure_path = "figures/accelerate_effectiveness.png"
min_bandwidth = 5e8
max_bandwidth = 10e9

with open(file_path, "r") as f:
    data = json.load(f)


def visualize_all_models():
    n_row = math.ceil(len(data) / 4)
    fig, axs = plt.subplots(n_row, 4, figsize=(4*3, n_row*2))
    for idx, (model, series) in enumerate(data.items()):
        model = model.split("/")[-1][:-5]
        bandwidth = [x[0]/1e9 for x in series if min_bandwidth <= x[0] <= max_bandwidth]
        jct = [x[1] for x in series if min_bandwidth <= x[0] <= max_bandwidth]
        axs[idx//4, idx%4].plot(bandwidth, jct)
        axs[idx//4, idx%4].set_title(model)
        axs[idx//4, idx%4].set_xlabel("Algo Bandwidth (GB/s)")
        axs[idx//4, idx%4].set_ylabel("JCT (s)")
    fig.tight_layout()
    plt.savefig(figure_path, dpi=144)


def visualize_model(model: str):
    series = data[model]
    model = model.split("/")[-1][:-5]
    bandwidth = [x[0]/1e9 for x in series if min_bandwidth <= x[0] <= max_bandwidth]
    jct = [x[1] for x in series if min_bandwidth <= x[0] <= max_bandwidth]
    plt.plot(bandwidth, jct)
    plt.title(model)
    plt.xlabel("Algorithm Bandwidth (GB/s)")
    plt.ylabel("JCT (s)")
    plt.savefig(figure_path, dpi=144)


if __name__ == "__main__":
    visualize_all_models()
    # visualize_model("../data/opt-350m-16.json")
