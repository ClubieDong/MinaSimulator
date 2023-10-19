import numpy as np
import matplotlib.pyplot as plt
import random

line = input()
data = [int(x) for x in line.split()]
jobIds = list(set(data))
random.shuffle(jobIds)
jobIdMap = {jobId: i for i, jobId in enumerate(jobIds)}
jobIdMap[0] = 0
data = [jobIdMap[jobId] / max(data) for jobId in data]
data = np.array(data).reshape(-1, 64)

plt.figure(figsize=(12, 4))
plt.imshow(data, cmap="nipy_spectral")
plt.savefig("figures/cluster_status.png", dpi=300)
