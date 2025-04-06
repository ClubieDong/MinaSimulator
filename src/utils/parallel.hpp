#pragma once

#include <array>
#include <future>

class Parallel {
public:
    template <typename TRes, typename... TFuncs>
    static std::array<TRes, sizeof...(TFuncs)> Run(TFuncs &&...funcs) {
        std::array futures = {std::async(std::launch::async, std::forward<TFuncs>(funcs))...};
        std::array<TRes, sizeof...(TFuncs)> results;
        for (unsigned int i = 0; i < futures.size(); ++i)
            results[i] = std::move(futures[i].get());
        return results;
    }

    template <typename TRes, typename TFuncs>
    static std::vector<TRes> RunRanks(const TFuncs &funcs, unsigned int count) {
        std::vector<std::future<TRes>> futures;
        futures.reserve(count);
        for (unsigned int i = 0; i < count; ++i)
            futures.push_back(std::async(std::launch::async, funcs, i));
        std::vector<TRes> results;
        results.reserve(count);
        for (unsigned int i = 0; i < count; ++i)
            results.push_back(std::move(futures[i].get()));
        return results;
    }
};
