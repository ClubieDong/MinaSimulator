#include "allocation_methods/host_tree.hpp"
#include "allocation_methods/hosts/first.hpp"
#include "allocation_methods/hosts/random.hpp"
#include "allocation_methods/hosts/smart.hpp"
#include "allocation_methods/trees/first.hpp"
#include "allocation_methods/trees/random.hpp"
#include <fstream>
#include <functional>
#include <iostream>
#include <nlohmann/json.hpp>

using AllocatorFunc = std::function<std::pair<AllocRes, std::optional<FatTree<3>::AggrTree>>(unsigned int)>;
using AllocatorCreator = std::function<AllocatorFunc(const FatTreeResource<3> &)>;

std::vector<std::pair<std::string, AllocatorCreator>> Allocators = [] {
    std::vector<std::pair<std::string, AllocatorCreator>> result = {
        std::pair("Random hosts + random tree",
                  [](const FatTreeResource<3> &resources) {
                      return HostTreeAllocator(resources, RandomHostsAllocator(resources),
                                               RandomTreeAllocator(resources));
                  }),
        // std::pair("Random hosts + first tree",
        //           [](const FatTreeResource<3> &resources) {
        //               return HostTreeAllocator(resources, RandomHostsAllocator(resources),
        //                                        FirstTreeAllocator(resources));
        //           }),
        std::pair("First hosts + random tree",
                  [](const FatTreeResource<3> &resources) {
                      return HostTreeAllocator(resources, FirstHostsAllocator(resources),
                                               RandomTreeAllocator(resources));
                  }),
        // std::pair("First hosts + first tree",
        //           [](const FatTreeResource<3> &resources) {
        //               return HostTreeAllocator(resources, FirstHostsAllocator(resources),
        //                                        FirstTreeAllocator(resources));
        //           }),
    };
    for (unsigned int i = 0; i <= 20; ++i) {
        double alpha = i / 10.0;
        result.emplace_back("Smart hosts (" + std::to_string(alpha) + ") + random tree",
                            [alpha](const FatTreeResource<3> &resources) {
                                return HostTreeAllocator(resources, SmartHostsAllocator(resources, alpha),
                                                         FirstTreeAllocator(resources));
                            });
    }
    return result;
}();

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

std::pair<std::vector<unsigned int>, std::vector<double>> GetAllTraces() {
    std::ifstream file("traces.json");
    auto json = nlohmann::json::parse(file);
    std::vector<unsigned int> nHostsTraces;
    for (const auto &trace : json["n_hosts"])
        for (unsigned int data : trace)
            nHostsTraces.push_back(data);
    std::vector<double> durationTraces;
    for (const auto &trace : json["duration"])
        for (double data : trace)
            durationTraces.push_back(data);
    return {nHostsTraces, durationTraces};
}

void TestAll() {
    std::vector<unsigned int> nHostsTraces;
    std::vector<double> durationTraces;
    std::tie(nHostsTraces, durationTraces) = GetAllTraces();
    thread_local std::default_random_engine engine(std::random_device{}());
    std::uniform_int_distribution<std::size_t> nHostsRandom(0, nHostsTraces.size() - 1);
    std::uniform_int_distribution<std::size_t> durationRandom(0, durationTraces.size() - 1);
    auto nHostSampleFunc = [&] { return nHostsTraces[nHostsRandom(engine)]; };
    auto durationSampleFunc = [&] { return durationTraces[durationRandom(engine)]; };
    for (auto [name, allocatorCreator] : Allocators) {
        std::cout << "==================== " << name << " ====================\n";
        for (unsigned int i = 1; i <= 8; ++i) {
            for (unsigned int j = 1; j <= 8; ++j) {
                FatTree<3> topology({8, 8, 16}, {1, i, j});
                FatTreeResource resources(topology, std::nullopt, 1);
                auto allocator = allocatorCreator(resources);
                auto result = Simulate(resources, 10000, allocator, nHostSampleFunc, durationSampleFunc);
                std::cout << 1.0 - result << ' ' << std::flush;
            }
            std::cout << '\n' << std::flush;
        }
        std::cout << '\n' << std::flush;
    }
}

int main() {
    TestAll();
    return 0;
}
