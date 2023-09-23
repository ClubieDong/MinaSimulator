#include "allocation_methods/host_tree.hpp"
#include "allocation_methods/hosts/first.hpp"
#include "allocation_methods/hosts/random.hpp"
#include "allocation_methods/trees/first.hpp"
#include "allocation_methods/trees/random.hpp"
#include <fstream>
#include <functional>
#include <iostream>
#include <nlohmann/json.hpp>

using AllocatorFunc = std::function<std::pair<AllocRes, std::optional<FatTree<3>::AggrTree>>(unsigned int)>;
using AllocatorCreator = std::function<AllocatorFunc(const FatTreeResource<3> &)>;

std::array<std::pair<const char *, AllocatorCreator>, 4> Allocators = {
    std::pair("Random hosts + random tree",
              [](const FatTreeResource<3> &resources) {
                  return HostTreeAllocator(resources, RandomHostsAllocator(resources), RandomTreeAllocator(resources));
              }),
    std::pair("Random hosts + first tree",
              [](const FatTreeResource<3> &resources) {
                  return HostTreeAllocator(resources, RandomHostsAllocator(resources), FirstTreeAllocator(resources));
              }),
    std::pair("First hosts + random tree",
              [](const FatTreeResource<3> &resources) {
                  return HostTreeAllocator(resources, FirstHostsAllocator(resources), RandomTreeAllocator(resources));
              }),
    std::pair("First hosts + first tree",
              [](const FatTreeResource<3> &resources) {
                  return HostTreeAllocator(resources, FirstHostsAllocator(resources), FirstTreeAllocator(resources));
              }),
};

template <unsigned int Height, typename Allocator, typename NHostSampleFunc, typename DurationSampleFunc>
double Simulate(FatTreeResource<Height> &resources, unsigned int nJobs, Allocator &allocator,
                NHostSampleFunc &nHostSampleFunc, DurationSampleFunc &durationSampleFunc) {
    std::priority_queue<std::pair<float, unsigned int>, std::vector<std::pair<float, unsigned int>>,
                        std::greater<std::pair<float, unsigned int>>>
        jobQueue;
    unsigned int nAllocatedJobs = 0;
    double now = 0.0, sharpGpuTime = 0.0, totalGpuTime = 0.0;
    auto nHosts = nHostSampleFunc();
    auto duration = durationSampleFunc();
    while (true) {
        while (true) {
            auto [res, tree] = allocator(nHosts);
            if (res == AllocRes::Fail)
                break;
            resources.Allocate(nAllocatedJobs, std::move(*tree));
            jobQueue.push({now + duration, nAllocatedJobs});
            if (res == AllocRes::Sharp)
                sharpGpuTime += nHosts * duration;
            totalGpuTime += nHosts * duration;
            ++nAllocatedJobs;
            if (nAllocatedJobs >= nJobs)
                return sharpGpuTime / totalGpuTime;
            nHosts = nHostSampleFunc();
            duration = durationSampleFunc();
        }
        assert(!jobQueue.empty());
        now = jobQueue.top().first;
        resources.Deallocate(jobQueue.top().second);
        jobQueue.pop();
    }
}

int main() {
    std::ifstream file("traces.json");
    auto json = nlohmann::json::parse(file);
    std::vector<unsigned int> nHostsTrace = json["n_hosts"][0];
    std::vector<double> durationTrace = json["duration"][0];
    thread_local std::default_random_engine engine(std::random_device{}());
    std::uniform_int_distribution<std::size_t> nHostsRandom(0, nHostsTrace.size() - 1);
    std::uniform_int_distribution<std::size_t> durationRandom(0, durationTrace.size() - 1);
    auto nHostSampleFunc = [&] { return nHostsTrace[nHostsRandom(engine)]; };
    auto durationSampleFunc = [&] { return durationTrace[durationRandom(engine)]; };
    FatTree<3> topology(16);
    for (auto [name, allocatorCreator] : Allocators) {
        FatTreeResource resources(topology, std::nullopt, 1);
        auto allocator = allocatorCreator(resources);
        auto result = Simulate(resources, 10000, allocator, nHostSampleFunc, durationSampleFunc);
        std::cout << name << ": " << result << '\n';
    }
    return 0;
}
