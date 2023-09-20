#include "job_allocator.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

template <unsigned int Height, typename NHostSampleFunc, typename DurationSampleFunc>
double Simulate(JobAllocator<Height> &&jobAllocator, unsigned int nJobs,
                typename JobAllocator<Height>::AllocHostMethod allocHostMethod,
                typename JobAllocator<Height>::AllocTreeMethod allocTreeMethod, NHostSampleFunc &&nHostSampleFunc,
                DurationSampleFunc &&durationSampleFunc) {
    std::priority_queue<std::pair<float, unsigned int>, std::vector<std::pair<float, unsigned int>>,
                        std::greater<std::pair<float, unsigned int>>>
        jobQueue;
    unsigned int nAllocatedJobs = 0;
    double now = 0.0, sharpGpuTime = 0.0, totalGpuTime = 0.0;
    auto nHosts = nHostSampleFunc();
    auto duration = durationSampleFunc();
    while (true) {
        while (true) {
            auto [res, tree] = jobAllocator.TryAllocateHostsAndTree(nHosts, allocHostMethod, allocTreeMethod);
            if (res == JobAllocator<Height>::AllocRes::Fail)
                break;
            jobAllocator.DoAllocate(nAllocatedJobs, std::move(*tree));
            jobQueue.push({now + duration, nAllocatedJobs});
            if (res == JobAllocator<Height>::AllocRes::Sharp)
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
        jobAllocator.DoDeallocate(jobQueue.top().second);
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
    for (auto [allocHostMethodName, allocHostMethod] : JobAllocator<3>::AllocHostMethods)
        for (auto [allocTreeMethodName, allocTreeMethod] : JobAllocator<3>::AllocTreeMethods) {
            auto result = Simulate(JobAllocator(topology, std::nullopt, 1), 10000, allocHostMethod, allocTreeMethod,
                                   std::move(nHostSampleFunc), std::move(durationSampleFunc));
            std::cout << allocHostMethodName << " + " << allocTreeMethodName << ": " << result << '\n';
        }
    return 0;
}
