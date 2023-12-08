import json
import math
from matplotlib import pyplot as plt


file_path_list = [
    "build/sharing_policy_100Gbps_1.2.json",
    "build/sharing_policy_200Gbps_1.2.json",
    "build/sharing_policy_400Gbps_1.2.json",
    "build/sharing_policy_100Gbps_1.5.json",
    "build/sharing_policy_200Gbps_1.5.json",
    "build/sharing_policy_400Gbps_1.5.json",
]
figure_path_list = [
    "figures/sharing_policy_100Gbps_1.2.png",
    "figures/sharing_policy_200Gbps_1.2.png",
    "figures/sharing_policy_400Gbps_1.2.png",
    "figures/sharing_policy_100Gbps_1.5.png",
    "figures/sharing_policy_200Gbps_1.5.png",
    "figures/sharing_policy_400Gbps_1.5.png",
]
sharing_policy_list = ["smart", "greedy"]


def draw_figure(file_path: str, figure_path: str):
    with open(file_path, "r") as f:
        data = json.load(f)
    model_list = data["model_list"]
    model_list = [x.split("/")[-1][:-5] for x in model_list]
    bandwidth = data["bandwidth"]
    sharp_acc_atio = data["sharp_acc_atio"]
    results = [[[x or math.nan for x in row] for row in data[name]] for name in sharing_policy_list]
    average_score = {name: value or math.nan for name, value in data["average_score"].items()}

    fig, axs = plt.subplots(1, len(sharing_policy_list), figsize=(len(sharing_policy_list)*5, 5))
    for idx, (sharing_policy, result) in enumerate(zip(sharing_policy_list, results)):
        axs[idx].imshow(result, vmin=0.5, vmax=1.0)
        axs[idx].set_xticks(range(len(model_list)))
        axs[idx].set_yticks(range(len(model_list)))
        axs[idx].set_xticklabels(model_list, rotation=45)
        axs[idx].set_yticklabels(model_list)
        for i in range(len(result)):
            for j in range(len(result[i])):
                axs[idx].text(j, i, f"{result[i][j]:.2f}", ha="center", va="center", color="w" if result[i][j] < 0.75 else "k")
        axs[idx].set_title(f"{sharing_policy} Bw={bandwidth/1e9:.1f}GB/s Acc={sharp_acc_atio:.1f} Avg={average_score[sharing_policy]:.2f}")
    fig.tight_layout()
    plt.savefig(figure_path, dpi=144)


def main():
    for file_path, figure_path in zip(file_path_list, figure_path_list):
        draw_figure(file_path, figure_path)


if __name__ == "__main__":
    main()
