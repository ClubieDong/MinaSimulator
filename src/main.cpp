#include "experiments/experiments.hpp"

int main(int argc, const char *argv[]) {
    if (argc != 2)
        return 1;
    std::string name = argv[1];
    if (name == "large-scale-simulation")
        TestLargeScaleSimulation();
    else if (name == "ablation-study")
        TestAblationStudy();
    else if (name == "sharing")
        TestSharing();
    else if (name == "tree-building")
        TestTreeBuilding();
    else if (name == "tree-conflicts")
        TestTreeConflicts();
    else if (name == "job-placement")
        TestJobPlacement();
    else if (name == "accelerate-effectiveness")
        TestAccelerateEffectiveness();
    else if (name == "sharing-overhead")
        TestSharingOverhead();
    return 0;
}
