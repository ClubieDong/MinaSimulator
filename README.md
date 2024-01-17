# MinaSimulator

## Setup

```
git submodule update --init
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install nlohmann-json
```

## Experiments

### TestLargeScaleSimulation

```
mina_sim large-scale-simulation
python scripts/visualize_large_scale_simulation.py
```

### TestAblationStudy

```
mina_sim ablation-study
```

### TestSharing

```
mina_sim sharing
python scripts/visualize_sharing_policy.py
```

### TestTreeBuilding

```
mina_sim tree-building
python scripts/visualize_tree_building.py
```

### TestTreeConflicts

```
mina_sim tree_conflicts
python scripts/visualize_tree_conflicts.py
```

### TestJobPlacement

```
mina_sim --name job-placement
python scripts/visualize_job_placement.py
```

### TestAccelerateEffectiveness

```
mina_sim --name accelerate-effectiveness
python scripts/visualize_accelerate_effectiveness.py
```

### TestSharingOverhead

```
mina_sim --name sharing-overhead
```
