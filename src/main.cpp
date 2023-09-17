#include "fat_tree.hpp"
#include "job_allocator.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <queue>
#include <random>

template <unsigned int Height, typename NHostSampleFunc, typename DurationSampleFunc>
double Simulate(JobAllocator<3> &&jobAllocator, unsigned int nJobs, JobAllocator<3>::AllocMethod method,
                NHostSampleFunc &&nHostSampleFunc, DurationSampleFunc &&durationSampleFunc) {
    std::priority_queue<std::pair<float, unsigned int>, std::vector<std::pair<float, unsigned int>>,
                        std::greater<std::pair<float, unsigned int>>>
        jobQueue;
    unsigned int nAllocatedJobs = 0;
    double now = 0.0, sharpGpuTime = 0.0, totalGpuTime = 0.0;
    auto nHosts = nHostSampleFunc();
    auto duration = durationSampleFunc();
    while (true) {
        while (true) {
            auto [res, jobId] = jobAllocator.Allocate(nHosts, method);
            if (res == JobAllocator<3>::AllocRes::Fail)
                break;
            jobQueue.push({now + duration, jobId});
            if (res == JobAllocator<3>::AllocRes::Sharp)
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
        jobAllocator.Deallocate(jobQueue.top().second);
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
    auto result = Simulate<3>(JobAllocator(FatTree<3>(16), std::nullopt, 1), 10000, &JobAllocator<3>::AllocateRandom,
                              std::move(nHostSampleFunc), std::move(durationSampleFunc));
    std::cout << result << '\n';
    return 0;
}
