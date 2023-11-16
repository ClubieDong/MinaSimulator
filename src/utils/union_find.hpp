#pragma once

#include <vector>

class UnionFind {
private:
    std::vector<unsigned int> m_Parents;

public:
    explicit UnionFind(unsigned int size) : m_Parents(size) {
        for (unsigned int i = 0; i < size; ++i)
            m_Parents[i] = i;
    }

    unsigned int Find(unsigned int x) {
        if (m_Parents[x] == x)
            return x;
        return m_Parents[x] = Find(m_Parents[x]);
    }

    void Union(unsigned int x, unsigned int y) {
        x = Find(x);
        y = Find(y);
        if (x != y)
            m_Parents[x] = y;
    }

    std::unordered_map<unsigned int, std::vector<unsigned int>> Group() {
        std::unordered_map<unsigned int, std::vector<unsigned int>> groups;
        for (unsigned int i = 0; i < m_Parents.size(); ++i)
            groups[Find(i)].push_back(i);
        return groups;
    }
};
