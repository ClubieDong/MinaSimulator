#include "union_find.hpp"

UnionFind::UnionFind(unsigned int size) : m_Parents(size), m_Rank(size, 0) {
    for (unsigned int i = 0; i < size; ++i)
        m_Parents[i] = i;
}

unsigned int UnionFind::Find(unsigned int x) {
    if (m_Parents[x] == x)
        return x;
    return m_Parents[x] = Find(m_Parents[x]);
}

void UnionFind::Union(unsigned int x, unsigned int y) {
    auto rootX = Find(x);
    auto rootY = Find(y);
    if (rootX == rootY)
        return;
    if (m_Rank[rootX] < m_Rank[rootY])
        m_Parents[rootX] = rootY;
    else if (m_Rank[rootY] < m_Rank[rootX])
        m_Parents[rootY] = rootX;
    else {
        m_Parents[rootX] = rootY;
        ++m_Rank[rootY];
    }
}

std::unordered_map<unsigned int, std::vector<unsigned int>> UnionFind::Group() {
    std::unordered_map<unsigned int, std::vector<unsigned int>> groups;
    for (unsigned int i = 0; i < m_Parents.size(); ++i)
        groups[Find(i)].push_back(i);
    return groups;
}
