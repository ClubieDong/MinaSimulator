#pragma once

#include <unordered_map>
#include <vector>

class UnionFind {
private:
    std::vector<unsigned int> m_Parents;

public:
    explicit UnionFind(unsigned int size);

    unsigned int Find(unsigned int x);
    void Union(unsigned int x, unsigned int y);

    std::unordered_map<unsigned int, std::vector<unsigned int>> Group();
};
