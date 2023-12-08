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
};
