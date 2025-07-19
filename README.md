# MINA: Fine-grained In-network Aggregation Resource Scheduling for Machine Learning Service

> [!NOTE]
> This branch is the version for my graduate thesis. Please refer to the [`ToN`](https://github.com/ClubieDong/MinaSimulator/tree/ToN) branch for the latest version of this codebase.

This is the official repository to reproduce the simulation results of the paper "MINA: Fine-grained In-network Aggregation Resource Scheduling for Machine Learning Service" (accepted by INFOCOM 2025 [[link](https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=11044657)]; submitted to ToN 2025).

## Setup environment

```bash
git submodule update --init
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install nlohmann-json
```

To generate figures, a Python environment with `numpy`, `matplotlib`, and `tqdm` is required.

All experiments are compiled with Apple clang v12.0.5 targeting arm64-apple-darwin24.1.0 and run on MacBook Pro M1. Although experiments are deterministic, the results may vary on different platforms.

## Build

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel
cd ..
```

## Experiments

Before running the experiments, make sure the working directory is the root of this project.

### Tree Conflicts

For Figure "Relationship between tree conflicts and host fragments".

```bash
build/mina_sim tree-conflicts
python scripts/visualize_tree_conflicts.py
```

### Accelerate Effectiveness

For Figure "Relationship between algorithm bandwidth and duration of one training step".

```bash
build/mina_sim accelerate-effectiveness
python scripts/visualize_accelerate_effectiveness.py
```

### Output traces

```bash
build/mina_sim output-traces
python scripts/visualize_traces.py
```

### Large Scale Simulation

For Figure "Overall performance of MINA".

```bash
build/mina_sim large-scale-simulation
python scripts/visualize_large_scale_simulation.py
```

### Ablation Study

For Table "Results of ablation study".

```bash
build/mina_sim ablation-study
```

### Job Placement

For Figure "Performance of job placement algorithm with different oversubscription ratios".

```bash
build/mina_sim job-placement
python scripts/visualize_job_placement.py
```

### Tree Building

For Figure "Performance and overhead of tree building algorithm".

```bash
build/mina_sim tree-building
python scripts/visualize_tree_building.py
```

### INA Resource Sharing

For Figure "Sharing performance".

```bash
build/mina_sim sharing
python scripts/visualize_sharing_policy.py
```

### Sharing Overhead

For "Overhead of host coordination" in Section 6.4 "Overhead Measurement".

```bash
build/mina_sim sharing-overhead
```

### Record cluster state

```bash
build/mina_sim record-cluster-state
python scripts/visualize_cluster_state.py
```
