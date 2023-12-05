#include "mis_solver.hpp"
#include <algorithm>
#include <cassert>

void MisSolver::Shrink(unsigned int u, unsigned int &end, const std::vector<char> &is) {
    auto i = m_NodeOffsets[u];
    while (true) {
        while (i < end && is[m_Edges[i]])
            ++i;
        while (i < end && !is[m_Edges[end - 1]])
            --end;
        if (i >= end)
            break;
        std::swap(m_Edges[i], m_Edges[end - 1]);
    }
}

void MisSolver::Shrink(unsigned int u, unsigned int &end, const std::vector<char> &is,
                       std::vector<unsigned int>::iterator tri) {
    auto i = m_NodeOffsets[u];
    while (true) {
        while (i < end && is[m_Edges[i]])
            ++i;
        while (i < end && !is[m_Edges[end - 1]])
            --end;
        if (i >= end)
            break;
        std::swap(m_Edges[i], m_Edges[end - 1]);
        std::swap(tri[i], tri[end - 1]);
    }
}

void MisSolver::DeleteVertex(unsigned int v, std::vector<char> &is, std::vector<int> &degree,
                             std::vector<unsigned int> &degreeOnes, std::vector<unsigned int> &degreeTwos) {
    is[v] = false;
    for (auto k = m_NodeOffsets[v]; k < m_NodeOffsets[v + 1]; ++k)
        if (is[m_Edges[k]]) {
            auto w = m_Edges[k];
            --degree[w];
            if (degree[w] == 1)
                degreeOnes.push_back(w);
            else if (degree[w] == 2)
                degreeTwos.push_back(w);
        }
}

void MisSolver::DeleteVertex(unsigned int v, const std::vector<unsigned int> &pend, std::vector<char> &is,
                             std::vector<int> &degree, std::vector<unsigned int> &degreeOnes,
                             std::vector<unsigned int> &degreeTwos) {
    is[v] = false;
    for (auto k = m_NodeOffsets[v]; k < pend[v]; ++k)
        if (is[m_Edges[k]]) {
            auto w = m_Edges[k];
            --degree[w];
            if (degree[w] == 1)
                degreeOnes.push_back(w);
            else if (degree[w] == 2)
                degreeTwos.push_back(w);
        }
}

void MisSolver::DeleteVertex(unsigned int u, std::vector<unsigned int> &pend, std::vector<char> &is,
                             std::vector<unsigned int> &degreeTwos, std::vector<unsigned int>::iterator tri,
                             std::vector<char> &adj, std::vector<int> &degree, std::vector<char> &dominate,
                             std::vector<unsigned int> &dominated) {
    is[u] = false;
    Shrink(u, pend[u], is, tri);
    for (auto i = m_NodeOffsets[u]; i < pend[u]; ++i) {
        auto v = m_Edges[i];
        adj[v] = true;
        --degree[v];
        if (degree[v] == 2)
            degreeTwos.push_back(v);
    }
    for (auto i = m_NodeOffsets[u]; i < pend[u]; ++i) {
        auto v = m_Edges[i];
        for (auto j = m_NodeOffsets[v]; j < pend[v]; ++j)
            if (is[m_Edges[j]]) {
                auto w = m_Edges[j];
                if (adj[w])
                    --tri[j];
                if (static_cast<int>(tri[j]) + 1 == degree[v] && !dominate[w]) {
                    dominate[w] = true;
                    dominated.push_back(w);
                }
            }
    }
    for (auto i = m_NodeOffsets[u]; i < pend[u]; ++i)
        adj[m_Edges[i]] = false;
}

bool MisSolver::ExistEdge(unsigned int u1, unsigned int u2) {
    if (m_NodeOffsets[u1 + 1] - m_NodeOffsets[u1] < m_NodeOffsets[u2 + 1] - m_NodeOffsets[u2]) {
        for (auto i = m_NodeOffsets[u1]; i < m_NodeOffsets[u1 + 1]; ++i)
            if (m_Edges[i] == u2)
                return true;
        return false;
    }
    for (auto i = m_NodeOffsets[u2]; i < m_NodeOffsets[u2 + 1]; ++i)
        if (m_Edges[i] == u1)
            return true;
    return false;
}

bool MisSolver::ExistEdge(unsigned int u, unsigned int v, const std::vector<unsigned int> &pend) {
    if (pend[u] - m_NodeOffsets[u] < pend[v] - m_NodeOffsets[v]) {
        for (auto i = m_NodeOffsets[u]; i < pend[u]; ++i)
            if (m_Edges[i] == v)
                return true;
        return false;
    }
    for (auto i = m_NodeOffsets[v]; i < pend[v]; ++i)
        if (m_Edges[i] == u)
            return true;
    return false;
}

void MisSolver::EdgeRewire(unsigned int u, unsigned int u1, unsigned int u2) {
    for (auto i = m_NodeOffsets[u]; i < m_NodeOffsets[u + 1]; ++i)
        if (m_Edges[i] == u1) {
            m_Edges[i] = u2;
            break;
        }
}

void MisSolver::EdgeRewrite(unsigned int u, const std::vector<unsigned int> &pend, unsigned int v, unsigned int w) {
    for (auto i = m_NodeOffsets[u]; i < pend[u]; ++i)
        if (m_Edges[i] == v) {
            m_Edges[i] = w;
            return;
        }
}

void MisSolver::InitialDominanceAndDegreeTwoRemove(std::vector<unsigned int> &degreeOnes,
                                                   std::vector<unsigned int> &degreeTwos, std::vector<char> &is,
                                                   std::vector<int> &degree, std::vector<char> &adj,
                                                   std::vector<std::pair<unsigned int, unsigned int>> &S) {
    RemoveDegreeOneTwo(degreeOnes, degreeTwos, is, degree, S);
    // Sort vertices in degree increasing order
    std::vector<unsigned int> ids(m_NodeCount, 0);
    for (unsigned int i = 0; i < m_NodeCount; ++i)
        ++ids[degree[i]];
    for (unsigned int i = 1; i < m_NodeCount; ++i)
        ids[i] += ids[i - 1];
    std::vector<unsigned int> order(m_NodeCount);
    for (unsigned int i = 0; i < m_NodeCount; ++i) {
        order[i] = ids[degree[i]];
        --ids[degree[i]];
    }
    for (unsigned int i = 0; i < m_NodeCount; ++i)
        ids[order[i] - 1] = i;
    // Compute dominance for vertices in degree decreasing order
    for (int i = m_NodeCount - 1; i >= 0; --i) {
        auto u = ids[i];
        if (!is[u] || degree[u] <= 0)
            continue;
        for (auto j = m_NodeOffsets[u]; j < m_NodeOffsets[u + 1]; ++j)
            if (is[m_Edges[j]])
                adj[m_Edges[j]] = true;
        adj[u] = true;
        bool dominate = false;
        for (auto j = m_NodeOffsets[u]; j < m_NodeOffsets[u + 1]; ++j)
            if (is[m_Edges[j]] && degree[m_Edges[j]] <= degree[u]) {
                int triCnt = 0;
                auto v = m_Edges[j];
                for (auto k = m_NodeOffsets[v]; k < m_NodeOffsets[v + 1]; ++k)
                    if (is[m_Edges[k]]) {
                        if (!adj[m_Edges[k]])
                            break;
                        ++triCnt;
                    }
                if (triCnt == degree[v]) {
                    dominate = true;
                    break;
                }
            }
        for (auto j = m_NodeOffsets[u]; j < m_NodeOffsets[u + 1]; ++j)
            adj[m_Edges[j]] = false;
        adj[u] = false;
        if (dominate) {
            DeleteVertex(u, is, degree, degreeOnes, degreeTwos);
            RemoveDegreeOneTwo(degreeOnes, degreeTwos, is, degree, S);
        }
    }
}

void MisSolver::RemoveDegreeOneTwo(std::vector<unsigned int> &degreeOnes, std::vector<unsigned int> &degreeTwos,
                                   std::vector<char> &is, std::vector<int> &degree,
                                   std::vector<std::pair<unsigned int, unsigned int>> &S) {
    while (!degreeOnes.empty() || !degreeTwos.empty()) {
        while (!degreeOnes.empty()) {
            auto u = degreeOnes.back();
            degreeOnes.pop_back();
            if (!is[u] || degree[u] != 1)
                continue;
            for (auto j = m_NodeOffsets[u]; j < m_NodeOffsets[u + 1]; ++j)
                if (is[m_Edges[j]])
                    DeleteVertex(m_Edges[j], is, degree, degreeOnes, degreeTwos);
        }
        while (!degreeTwos.empty() && degreeOnes.empty()) {
            auto u = degreeTwos.back();
            degreeTwos.pop_back();
            if (!is[u] || degree[u] != 2)
                continue;
            auto u1 = m_NodeCount, u2 = m_NodeCount;
            GetTwoNeighbors(u, is, u1, u2);
            assert(u1 != m_NodeCount && u2 != m_NodeCount);
            unsigned int pre = u, cnt = 1;
            while (u1 != u && degree[u1] == 2) {
                ++cnt;
                auto tmp = GetOtherNeighbor(u1, is, pre);
                pre = u1;
                u1 = tmp;
            }
            if (u1 == u) {
                DeleteVertex(u, is, degree, degreeOnes, degreeTwos);
                assert(!degreeOnes.empty());
                continue;
            }
            pre = u;
            while (degree[u2] == 2) {
                ++cnt;
                auto tmp = GetOtherNeighbor(u2, is, pre);
                pre = u2;
                u2 = tmp;
            }
            if (u1 == u2) {
                DeleteVertex(u1, is, degree, degreeOnes, degreeTwos);
                assert(!degreeOnes.empty());
                continue;
            }
            if (cnt % 2 == 1) {
                if (ExistEdge(u1, u2)) {
                    DeleteVertex(u1, is, degree, degreeOnes, degreeTwos);
                    DeleteVertex(u2, is, degree, degreeOnes, degreeTwos);
                    assert(!degreeOnes.empty());
                } else if (cnt > 1) {
                    u = GetOtherNeighbor(pre, is, u2);
                    EdgeRewire(pre, u, u1);
                    u2 = pre;
                    while (u != u1) {
                        is[u] = false;
                        auto tmp = GetOtherNeighbor(u, is, pre);
                        S.emplace_back(u, tmp);
                        pre = u;
                        u = tmp;
                    }
                    EdgeRewire(u1, pre, u2);
                }
            } else {
                auto v2 = pre, v1 = pre;
                pre = u2;
                while (v1 != u1) {
                    is[v1] = false;
                    auto tmp = GetOtherNeighbor(v1, is, pre);
                    S.emplace_back(v1, tmp);
                    pre = v1;
                    v1 = tmp;
                }
                v1 = pre;
                if (ExistEdge(u1, u2)) {
                    if (--degree[u1] == 2)
                        degreeTwos.push_back(u1);
                    if (--degree[u2] == 2)
                        degreeTwos.push_back(u2);
                    assert(degree[u1] > 1 && degree[u2] > 1);
                } else {
                    EdgeRewire(u1, v1, u2);
                    EdgeRewire(u2, v2, u1);
                }
            }
        }
    }
}

void MisSolver::GetTwoNeighbors(unsigned int u, const std::vector<char> &is, unsigned int &u1, unsigned int &u2) {
    for (auto i = m_NodeOffsets[u]; i < m_NodeOffsets[u + 1]; ++i)
        if (is[m_Edges[i]]) {
            if (u1 == m_NodeCount) {
                u1 = m_Edges[i];
                std::swap(m_Edges[m_NodeOffsets[u]], m_Edges[i]);
            } else {
                assert(u2 == m_NodeCount);
                u2 = m_Edges[i];
                std::swap(m_Edges[m_NodeOffsets[u] + 1], m_Edges[i]);
                break;
            }
        }
}

unsigned int MisSolver::GetOtherNeighbor(unsigned int u, const std::vector<char> &is, unsigned int u1) {
    unsigned int idx = 0;
    for (auto i = m_NodeOffsets[u]; i < m_NodeOffsets[u + 1]; ++i)
        if (is[m_Edges[i]]) {
            std::swap(m_Edges[m_NodeOffsets[u] + idx], m_Edges[i]);
            if (++idx == 2)
                break;
        }
    assert(m_Edges[m_NodeOffsets[u]] == u1 || m_Edges[m_NodeOffsets[u] + 1] == u1);
    if (m_Edges[m_NodeOffsets[u]] == u1)
        return m_Edges[m_NodeOffsets[u] + 1];
    return m_Edges[m_NodeOffsets[u]];
}

void MisSolver::LpReduction(const std::vector<unsigned int> &ids, unsigned int idsN, std::vector<char> &is,
                            std::vector<int> &degree) {
    std::vector<int> newId(m_NodeCount, -1);
    for (unsigned int i = 0; i < idsN; ++i)
        newId[ids[i]] = i;
    for (unsigned int i = 0; i < idsN; ++i) {
        auto u = ids[i];
        for (auto j = m_NodeOffsets[u]; j < m_NodeOffsets[u + 1]; ++j)
            m_Edges[j] = newId[m_Edges[j]];
    }
    std::vector<unsigned int> newNodeOffsets(idsN + 1);
    for (unsigned int i = 0; i < idsN; ++i)
        newNodeOffsets[i] = m_NodeOffsets[ids[i]];
    newNodeOffsets[idsN] = m_NodeOffsets[ids[idsN - 1] + 1];
    std::vector<int> x(idsN, -1);
    std::vector<int> y(idsN, -1);
    std::vector<char> used(2 * idsN);
    std::vector<int> level(2 * idsN);
    std::vector<unsigned int> que(2 * idsN);
    std::vector<unsigned int> iter(idsN);
    while (true) {
        std::fill(used.begin(), used.end(), false);
        unsigned int queN = 0;
        for (unsigned int u = 0; u < idsN; ++u)
            if (x[u] == -1) {
                level[u] = 0;
                used[u] = true;
                que[queN++] = u;
            }
        bool find = false;
        for (unsigned int i = 0; i < queN; ++i) {
            auto u = que[i];
            iter[u] = newNodeOffsets[u + 1];
            for (auto j = newNodeOffsets[u]; j < newNodeOffsets[u + 1]; ++j)
                if (!used[idsN + m_Edges[j]]) {
                    used[idsN + m_Edges[j]] = true;
                    auto v = y[m_Edges[j]];
                    if (v == -1)
                        find = true;
                    else {
                        assert(!used[v]);
                        used[v] = true;
                        level[v] = level[u] + 1;
                        que[queN++] = v;
                    }
                }
        }
        if (!find)
            break;
        for (unsigned int i = 0; i < idsN; ++i)
            if (x[i] == -1) {
                queN = 0;
                que[queN++] = i;
                while (queN > 0) {
                    auto u = que[queN - 1];
                    if (iter[u] <= newNodeOffsets[u]) {
                        --queN;
                        if (queN > 0)
                            --iter[que[queN - 1]];
                    } else {
                        auto v = m_Edges[iter[u] - 1];
                        if (y[v] == -1) {
                            for (unsigned int j = 0; j < queN; ++j) {
                                u = que[j];
                                v = m_Edges[iter[u] - 1];
                                x[u] = v;
                                y[v] = u;
                            }
                            queN = 0;
                        } else if (level[y[v]] > level[u])
                            que[queN++] = y[v];
                        else
                            --iter[u];
                    }
                }
            }
    }
    for (unsigned int u = 0; u < idsN; ++u)
        if (!used[u] && used[idsN + u]) {
            is[ids[u]] = false;
            for (auto j = newNodeOffsets[u]; j < newNodeOffsets[u + 1]; ++j)
                if (is[ids[m_Edges[j]]])
                    --degree[ids[m_Edges[j]]];
        }
    std::fill(used.begin(), used.end(), false);
    for (unsigned int i = 0; i < idsN; ++i)
        iter[i] = newNodeOffsets[i + 1];
    unsigned int levelN = 0;
    for (unsigned int u = 0; u < idsN; ++u)
        if (!used[u] && is[ids[u]] && degree[ids[u]] > 0) {
            unsigned int queN = 0;
            que[queN++] = u;
            used[u] = true;
            while (queN > 0) {
                u = que[queN - 1];
                if (u < idsN) {
                    assert(x[u] != -1 && is[ids[x[u]]] && degree[ids[x[u]]] > 0);
                    if (!used[x[u] + idsN]) {
                        used[x[u] + idsN] = true;
                        que[queN++] = x[u] + idsN;
                    } else {
                        level[levelN++] = u;
                        --queN;
                    }
                } else {
                    u -= idsN;
                    unsigned int v = -1;
                    while (iter[u] > newNodeOffsets[u]) {
                        v = m_Edges[iter[u] - 1];
                        if (!used[v] && is[ids[v]]) {
                            assert(degree[ids[v]] > 0);
                            break;
                        }
                        --iter[u];
                    }
                    if (iter[u] <= newNodeOffsets[u])
                        --queN;
                    else {
                        used[v] = true;
                        que[queN++] = v;
                    }
                }
            }
        }
    std::fill(used.begin(), used.end(), false);
    std::vector<char> in(idsN, false);
    for (int i = levelN - 1; i >= 0; --i) {
        auto u = level[i];
        if (used[u] || !is[ids[u]] || degree[ids[u]] == 0)
            continue;
        unsigned int queN = 0;
        que[queN++] = u;
        used[u] = true;
        bool ok = true;
        for (unsigned int j = 0; j < queN; ++j) {
            u = que[j];
            if (ok && used[u < static_cast<int>(idsN) ? (u + idsN) : (u - idsN)])
                ok = false;
            if (u < static_cast<int>(idsN))
                for (auto k = newNodeOffsets[u]; k < newNodeOffsets[u + 1]; ++k) {
                    auto v = m_Edges[k];
                    if (!is[ids[v]] || degree[ids[v]] <= 0)
                        continue;
                    v += idsN;
                    if (!used[v]) {
                        in[v - idsN] = true;
                        used[v] = true;
                        que[queN++] = v;
                    } else if (!in[v - idsN])
                        ok = false;
                }
            else {
                u -= idsN;
                assert(y[u] != -1 && is[ids[y[u]]] && degree[ids[y[u]]] > 0);
                if (!used[y[u]]) {
                    used[y[u]] = true;
                    que[queN++] = y[u];
                }
            }
        }
        for (unsigned int j = 0; j < queN; ++j)
            if (que[j] >= idsN)
                in[que[j] - idsN] = 0;
        if (ok)
            for (unsigned int j = 0; j < queN; ++j) {
                u = que[j];
                if (u >= static_cast<int>(idsN)) {
                    u -= idsN;
                    is[ids[u]] = false;
                    for (auto k = newNodeOffsets[u]; k < newNodeOffsets[u + 1]; ++k)
                        if (is[ids[m_Edges[k]]])
                            --degree[ids[m_Edges[k]]];
                }
            }
    }
    for (unsigned int i = 0; i < idsN; ++i) {
        auto u = ids[i];
        for (auto j = m_NodeOffsets[u]; j < m_NodeOffsets[u + 1]; ++j)
            m_Edges[j] = ids[m_Edges[j]];
    }
}

void MisSolver::ComputeTriangleCounts(std::vector<unsigned int>::iterator tri, std::vector<unsigned int> &pend,
                                      std::vector<char> &adj, std::vector<char> &is, std::vector<int> &degree,
                                      std::vector<char> &dominate, std::vector<unsigned int> &dominated) {
    std::vector<unsigned int> vs;
    for (unsigned int i = 0; i < m_NodeCount; ++i)
        if (is[i] && degree[i] > 0)
            vs.push_back(i);
    std::vector<unsigned int> ids(vs.size(), 0);
    for (unsigned int i = 0; i < vs.size(); ++i)
        ++ids[degree[vs[i]]];
    for (unsigned int i = 1; i < vs.size(); ++i)
        ids[i] += ids[i - 1];
    std::vector<int> order(m_NodeCount, -1);
    for (unsigned int i = 0; i < vs.size(); ++i)
        order[vs[i]] = --ids[degree[vs[i]]];
    for (unsigned int i = 0; i < vs.size(); ++i)
        ids[order[vs[i]]] = vs[i];
    for (auto i = static_cast<int>(vs.size()) - 1; i >= 0; --i) {
        auto u = ids[i];
        Shrink(u, pend[u], is, tri);
        for (auto j = m_NodeOffsets[u]; j < pend[u]; ++j)
            adj[m_Edges[j]] = true;
        if (!dominate[u]) {
            adj[u] = true;
            for (auto j = m_NodeOffsets[u]; j < pend[u]; ++j)
                if (degree[m_Edges[j]] <= degree[u]) {
                    int triCnt = 0;
                    auto v = m_Edges[j];
                    for (auto k = m_NodeOffsets[v]; k < pend[v]; ++k)
                        if (is[m_Edges[k]]) {
                            if (!adj[m_Edges[k]])
                                break;
                            ++triCnt;
                        }
                    if (triCnt == degree[v]) {
                        dominate[u] = true;
                        break;
                    }
                }
            adj[u] = false;
        }
        if (dominate[u]) {
            is[u] = false;
            for (auto j = m_NodeOffsets[u]; j < pend[u]; ++j)
                --degree[m_Edges[j]];
            for (auto j = m_NodeOffsets[u]; j < pend[u]; ++j)
                if (order[m_Edges[j]] > i) {
                    auto v = m_Edges[j];
                    for (auto k = m_NodeOffsets[v]; k < pend[v]; ++k)
                        if (is[m_Edges[k]]) {
                            auto w = m_Edges[k];
                            if (adj[w]) {
                                assert(tri[k] > 0);
                                --tri[k];
                            }
                            if (!dominate[v] && static_cast<int>(tri[k]) + 1 == degree[w]) {
                                dominate[v] = true;
                                dominated.push_back(v);
                            }
                            if (!dominate[w] && static_cast<int>(tri[k]) + 1 == degree[v]) {
                                dominate[w] = true;
                                if (order[w] > i)
                                    dominated.push_back(w);
                            }
                            assert(degree[v] > 1 || dominate[w]);
                            assert(degree[w] > 1 || dominate[v]);
                        }
                }
        } else
            for (auto j = m_NodeOffsets[u]; j < pend[u]; ++j) {
                auto v = m_Edges[j];
                tri[j] = 0;
                for (auto k = m_NodeOffsets[v]; k < pend[v]; ++k)
                    if (adj[m_Edges[k]])
                        ++tri[j];
                assert(static_cast<int>(tri[j]) + 1 != degree[v]);
                if (!dominate[v] && static_cast<int>(tri[j]) + 1 == degree[u]) {
                    dominate[v] = true;
                    if (order[v] > i)
                        dominated.push_back(v);
                }
            }
        for (auto j = m_NodeOffsets[u]; j < pend[u]; ++j)
            adj[m_Edges[j]] = false;
    }
}

bool MisSolver::DominatedCheck(unsigned int u, const std::vector<unsigned int> &pend, const std::vector<char> &is,
                               std::vector<unsigned int>::const_iterator tri, const std::vector<int> &degree) {
    for (unsigned int i = m_NodeOffsets[u]; i < pend[u]; ++i)
        if (is[m_Edges[i]] && static_cast<int>(tri[i]) + 1 == degree[m_Edges[i]])
            return true;
    return false;
}

void MisSolver::UpdateTriangle(unsigned int u1, unsigned int u2, std::vector<unsigned int> &pend, std::vector<char> &is,
                               std::vector<char> &adj, std::vector<unsigned int>::iterator tri,
                               const std::vector<int> &degree, std::vector<char> &dominate,
                               std::vector<unsigned int> &dominated) {
    int cnt = 0;
    Shrink(u1, pend[u1], is, tri);
    Shrink(u2, pend[u2], is, tri);
    for (auto i = m_NodeOffsets[u1]; i < pend[u1]; ++i)
        adj[m_Edges[i]] = true;
    for (auto i = m_NodeOffsets[u2]; i < pend[u2]; ++i)
        if (adj[m_Edges[i]]) {
            auto v = m_Edges[i];
            ++tri[i];
            ++cnt;
            if (static_cast<int>(tri[i]) + 1 == degree[u2] && !dominate[v]) {
                dominate[v] = true;
                dominated.push_back(v);
            }
            if (static_cast<int>(tri[i]) + 1 == degree[v] && !dominate[u2]) {
                dominate[u2] = true;
                dominated.push_back(u2);
            }
            for (auto j = m_NodeOffsets[v]; j < pend[v]; ++j)
                if (m_Edges[j] == u2) {
                    ++tri[j];
                    break;
                }
        }
    for (auto i = m_NodeOffsets[u1]; i < pend[u1]; ++i) {
        adj[m_Edges[i]] = false;
        if (m_Edges[i] == u2)
            tri[i] = cnt;
    }
    if (cnt + 1 == degree[u1] && !dominate[u2]) {
        dominate[u2] = true;
        dominated.push_back(u2);
    }
    if (cnt + 1 == degree[u2] && !dominate[u1]) {
        dominate[u1] = true;
        dominated.push_back(u1);
    }
    for (auto i = m_NodeOffsets[u2]; i < pend[u2]; ++i)
        adj[m_Edges[i]] = true;
    for (auto i = m_NodeOffsets[u1]; i < pend[u1]; ++i)
        if (adj[m_Edges[i]]) {
            auto v = m_Edges[i];
            ++tri[i];
            if (static_cast<int>(tri[i]) + 1 == degree[u1] && !dominate[v]) {
                dominate[v] = true;
                dominated.push_back(v);
            }
            if (static_cast<int>(tri[i]) + 1 == degree[v] && !dominate[u1]) {
                dominate[u1] = true;
                dominated.push_back(u1);
            }
            for (auto j = m_NodeOffsets[v]; j < pend[v]; ++j)
                if (m_Edges[j] == u1) {
                    ++tri[j];
                    break;
                }
        }
    for (auto i = m_NodeOffsets[u2]; i < pend[u2]; ++i) {
        adj[m_Edges[i]] = false;
        if (m_Edges[i] == u1)
            tri[i] = cnt;
    }
}

std::vector<char> MisSolver::LinearSolver() {
    std::vector<char> is(m_NodeCount, true);
    std::vector<int> binHead(m_NodeCount, -1);
    std::vector<int> binNext(m_NodeCount);
    std::vector<int> degree(m_NodeCount);
    std::vector<unsigned int> degreeOnes, degreeTwos;
    std::vector<std::pair<unsigned int, unsigned int>> S;
    int maxD = 0;
    for (unsigned int i = 0; i < m_NodeCount; ++i) {
        degree[i] = m_NodeOffsets[i + 1] - m_NodeOffsets[i];
        binNext[i] = binHead[degree[i]];
        binHead[degree[i]] = i;
        if (degree[i] == 1)
            degreeOnes.push_back(i);
        else if (degree[i] == 2)
            degreeTwos.push_back(i);
        if (degree[i] > maxD)
            maxD = degree[i];
    }
    std::vector<char> fixed(m_NodeCount, false);
    std::vector<unsigned int> pend(m_NodeCount);
    for (unsigned int i = 0; i < m_NodeCount; ++i)
        pend[i] = m_NodeOffsets[i + 1];
    bool firstTime = true;
    while (!degreeOnes.empty() || !degreeTwos.empty() || maxD >= 3) {
        while (!degreeOnes.empty() || !degreeTwos.empty()) {
            while (!degreeOnes.empty()) {
                auto u = degreeOnes.back();
                degreeOnes.pop_back();
                if (!is[u] || degree[u] != 1)
                    continue;
                for (auto j = m_NodeOffsets[u]; j < pend[u]; ++j)
                    if (is[m_Edges[j]])
                        DeleteVertex(m_Edges[j], pend, is, degree, degreeOnes, degreeTwos);
            }
            while (!degreeTwos.empty() && degreeOnes.empty()) {
                auto u = degreeTwos.back();
                degreeTwos.pop_back();
                if (!is[u] || degree[u] != 2)
                    continue;
                Shrink(u, pend[u], is);
                auto u1 = m_Edges[m_NodeOffsets[u]], u2 = m_Edges[m_NodeOffsets[u] + 1];
                unsigned int pre = u, cnt = 1;
                while (u1 != u && degree[u1] == 2) {
                    ++cnt;
                    Shrink(u1, pend[u1], is);
                    auto tmp = u1;
                    if (m_Edges[m_NodeOffsets[u1]] != pre)
                        u1 = m_Edges[m_NodeOffsets[u1]];
                    else
                        u1 = m_Edges[m_NodeOffsets[u1] + 1];
                    pre = tmp;
                }
                if (u1 == u) {
                    DeleteVertex(u, pend, is, degree, degreeOnes, degreeTwos);
                    continue;
                }
                pre = u;
                while (degree[u2] == 2) {
                    ++cnt;
                    Shrink(u2, pend[u2], is);
                    auto tmp = u2;
                    if (m_Edges[m_NodeOffsets[u2]] != pre)
                        u2 = m_Edges[m_NodeOffsets[u2]];
                    else
                        u2 = m_Edges[m_NodeOffsets[u2] + 1];
                    pre = tmp;
                }
                if (u1 == u2) {
                    DeleteVertex(u1, pend, is, degree, degreeOnes, degreeTwos);
                    continue;
                }
                Shrink(u1, pend[u1], is);
                Shrink(u2, pend[u2], is);
                if (cnt % 2 == 1) {
                    if (ExistEdge(u1, u2, pend)) {
                        DeleteVertex(u1, pend, is, degree, degreeOnes, degreeTwos);
                        DeleteVertex(u2, pend, is, degree, degreeOnes, degreeTwos);
                    } else if (cnt > 1) {
                        auto idx = m_NodeOffsets[pre];
                        if (m_Edges[idx] == u2)
                            ++idx;
                        u = m_Edges[idx];
                        m_Edges[idx] = u1;
                        u2 = pre;
                        while (u != u1) {
                            is[u] = 0;
                            auto tmp = u;
                            if (m_Edges[m_NodeOffsets[u]] == pre)
                                u = m_Edges[m_NodeOffsets[u] + 1];
                            else
                                u = m_Edges[m_NodeOffsets[u]];
                            S.emplace_back(tmp, u);
                            pre = tmp;
                        }
                        EdgeRewrite(u1, pend, pre, u2);
                    }
                } else {
                    auto v2 = pre, v1 = pre;
                    pre = u2;
                    while (v1 != u1) {
                        is[v1] = 0;
                        auto tmp = v1;
                        if (m_Edges[m_NodeOffsets[v1]] == pre)
                            v1 = m_Edges[m_NodeOffsets[v1] + 1];
                        else
                            v1 = m_Edges[m_NodeOffsets[v1]];
                        S.emplace_back(tmp, v1);
                        pre = tmp;
                    }
                    v1 = pre;
                    if (ExistEdge(u1, u2, pend)) {
                        --degree[u1];
                        --degree[u2];
                        if (degree[u1] == 2)
                            degreeTwos.push_back(u1);
                        if (degree[u2] == 2)
                            degreeTwos.push_back(u2);
                    } else {
                        EdgeRewrite(u1, pend, v1, u2);
                        EdgeRewrite(u2, pend, v2, u1);
                    }
                }
            }
        }
        if (firstTime) {
            firstTime = false;
            for (unsigned int k = 0; k < m_NodeCount; ++k)
                if (!is[k] || degree[k] <= 0)
                    fixed[k] = true;
        }
        while (degreeOnes.empty() && degreeTwos.empty()) {
            while (maxD >= 3 && binHead[maxD] == -1)
                --maxD;
            if (maxD < 3)
                break;
            int v = -1;
            for (v = binHead[maxD]; v != -1;) {
                auto tmp = binNext[v];
                if (is[v] && degree[v] > 0) {
                    if (degree[v] < maxD) {
                        binNext[v] = binHead[degree[v]];
                        binHead[degree[v]] = v;
                    } else {
                        S.emplace_back(v, m_NodeCount);
                        DeleteVertex(v, pend, is, degree, degreeOnes, degreeTwos);
                        binHead[maxD] = tmp;
                        break;
                    }
                }
                v = tmp;
            }
            if (v == -1)
                binHead[maxD] = -1;
        }
    }
    for (auto i = static_cast<int>(S.size()) - 1; i >= 0; --i) {
        auto u1 = S[i].first, u2 = S[i].second;
        assert(!is[u1]);
        if (u2 != m_NodeCount) {
            if (!is[u2])
                is[u1] = true;
            continue;
        }
        bool ok = true;
        for (auto i = m_NodeOffsets[u1]; i < m_NodeOffsets[u1 + 1]; ++i)
            if (is[m_Edges[i]]) {
                ok = false;
                break;
            }
        if (ok)
            is[u1] = true;
    }
    return is;
}

std::vector<char> MisSolver::NearLinearSolver() {
    std::vector<char> is(m_NodeCount, true);
    std::vector<char> adj(m_NodeCount, false);
    std::vector<unsigned int> degreeOnes, degreeTwos;
    std::vector<int> degree(m_NodeCount);
    for (unsigned int i = 0; i < m_NodeCount; ++i) {
        degree[i] = m_NodeOffsets[i + 1] - m_NodeOffsets[i];
        if (degree[i] == 1)
            degreeOnes.push_back(i);
        else if (degree[i] == 2)
            degreeTwos.push_back(i);
    }
    std::vector<std::pair<unsigned int, unsigned int>> S;
    InitialDominanceAndDegreeTwoRemove(degreeOnes, degreeTwos, is, degree, adj, S);
    std::vector<unsigned int> ids(m_NodeCount);
    unsigned int idsN = 0, newM = 0;
    for (unsigned int i = 0; i < m_NodeCount; ++i) {
        auto tmp = m_NodeOffsets[i];
        m_NodeOffsets[i] = newM;
        if (!is[i] || degree[i] <= 0)
            continue;
        ids[idsN++] = i;
        for (auto j = tmp; j < m_NodeOffsets[i + 1]; ++j)
            if (is[m_Edges[j]]) {
                assert(degree[m_Edges[j]] > 0);
                m_Edges[newM++] = m_Edges[j];
            }
    }
    m_NodeOffsets[m_NodeCount] = newM;
    if (idsN > 0)
        LpReduction(ids, idsN, is, degree);
    assert(degreeOnes.empty() && degreeTwos.empty());
    for (unsigned int i = 0; i < idsN; ++i)
        if (is[ids[i]] && degree[ids[i]] > 0) {
            if (degree[ids[i]] == 1)
                degreeOnes.push_back(ids[i]);
            else if (degree[ids[i]] == 2)
                degreeTwos.push_back(ids[i]);
        }
    RemoveDegreeOneTwo(degreeOnes, degreeTwos, is, degree, S);
    newM = 0;
    for (unsigned int i = 0; i < m_NodeCount; ++i) {
        auto tmp = m_NodeOffsets[i];
        m_NodeOffsets[i] = newM;
        if (!is[i] || degree[i] <= 0)
            continue;
        for (auto j = tmp; j < m_NodeOffsets[i + 1]; ++j)
            if (is[m_Edges[j]]) {
                assert(degree[m_Edges[j]] > 0);
                m_Edges[newM++] = m_Edges[j];
            }
    }
    m_NodeOffsets[m_NodeCount] = newM;
    std::vector<unsigned int> pend(m_NodeCount);
    for (unsigned int i = 0; i < m_NodeCount; ++i)
        pend[i] = m_NodeOffsets[i + 1];
    std::vector<unsigned int> triInner;
    std::vector<unsigned int>::iterator tri;
    if (newM <= m_EdgeCount / 2)
        tri = m_Edges.begin() + newM;
    else {
        triInner = std::vector<unsigned int>(m_NodeOffsets[m_NodeCount]);
        tri = triInner.begin();
    }
    std::vector<char> dominate(m_NodeCount, false);
    std::vector<unsigned int> dominated;
    ComputeTriangleCounts(tri, pend, adj, is, degree, dominate, dominated);
    std::vector<int> binHead(idsN, -1);
    std::vector<int> binNext(m_NodeCount);
    assert(degreeTwos.empty());
    int maxD = 0;
    for (unsigned int i = 0; i < m_NodeCount; ++i)
        if (is[i] && degree[i] > 0) {
            binNext[i] = binHead[degree[i]];
            binHead[degree[i]] = i;
            if (degree[i] == 2)
                degreeTwos.push_back(i);
            if (degree[i] > maxD)
                maxD = degree[i];
        }
    std::vector<char> fixed(m_NodeCount, 0);
    bool firstTime = true;
    while (!dominated.empty() || !degreeTwos.empty() || maxD >= 3) {
        while (!dominated.empty() || !degreeTwos.empty()) {
            while (!dominated.empty()) {
                auto u = dominated.back();
                dominated.pop_back();
                if (!is[u] || degree[u] == 0)
                    continue;
                if (!DominatedCheck(u, pend, is, tri, degree))
                    dominate[u] = false;
                else
                    DeleteVertex(u, pend, is, degreeTwos, tri, adj, degree, dominate, dominated);
            }
            while (!degreeTwos.empty() && dominated.empty()) {
                auto u = degreeTwos.back();
                degreeTwos.pop_back();
                if (!is[u] || degree[u] != 2)
                    continue;
                Shrink(u, pend[u], is, tri);
                assert(pend[u] == m_NodeOffsets[u] + 2);
                auto u1 = m_Edges[m_NodeOffsets[u]], u2 = m_Edges[m_NodeOffsets[u] + 1];
                unsigned int pre = u, cnt = 1;
                while (u1 != u && degree[u1] == 2) {
                    ++cnt;
                    Shrink(u1, pend[u1], is, tri);
                    assert(pend[u1] == m_NodeOffsets[u1] + 2);
                    auto tmp = u1;
                    if (m_Edges[m_NodeOffsets[u1]] != pre)
                        u1 = m_Edges[m_NodeOffsets[u1]];
                    else
                        u1 = m_Edges[m_NodeOffsets[u1] + 1];
                    pre = tmp;
                }
                if (u1 == u) {
                    DeleteVertex(u, pend, is, degreeTwos, tri, adj, degree, dominate, dominated);
                    assert(!dominated.empty());
                    continue;
                }
                pre = u;
                while (degree[u2] == 2) {
                    ++cnt;
                    Shrink(u2, pend[u2], is, tri);
                    assert(pend[u2] == m_NodeOffsets[u2] + 2);
                    auto tmp = u2;
                    if (m_Edges[m_NodeOffsets[u2]] != pre)
                        u2 = m_Edges[m_NodeOffsets[u2]];
                    else
                        u2 = m_Edges[m_NodeOffsets[u2] + 1];
                    pre = tmp;
                }
                if (u1 == u2) {
                    DeleteVertex(u1, pend, is, degreeTwos, tri, adj, degree, dominate, dominated);
                    assert(!dominated.empty());
                    continue;
                }
                if (cnt % 2 == 1) {
                    if (ExistEdge(u1, u2, pend)) {
                        DeleteVertex(u1, pend, is, degreeTwos, tri, adj, degree, dominate, dominated);
                        DeleteVertex(u2, pend, is, degreeTwos, tri, adj, degree, dominate, dominated);
                        assert(!dominated.empty());
                    } else if (cnt > 1) {
                        auto idx = m_NodeOffsets[pre];
                        if (m_Edges[idx] == u2)
                            ++idx;
                        assert(degree[pre] == 2 && tri[idx] == 0);
                        u = m_Edges[idx];
                        m_Edges[idx] = u1;
                        u2 = pre;
                        while (u != u1) {
                            is[u] = false;
                            auto tmp = u;
                            if (m_Edges[m_NodeOffsets[u]] == pre)
                                u = m_Edges[m_NodeOffsets[u] + 1];
                            else
                                u = m_Edges[m_NodeOffsets[u]];
                            pre = tmp;
                            S.emplace_back(pre, u);
                        }
                        EdgeRewrite(u1, pend, pre, u2);
                    }
                } else {
                    auto v2 = pre, v1 = pre;
                    pre = u2;
                    while (v1 != u1) {
                        is[v1] = false;
                        auto tmp = v1;
                        if (m_Edges[m_NodeOffsets[v1]] == pre)
                            v1 = m_Edges[m_NodeOffsets[v1] + 1];
                        else
                            v1 = m_Edges[m_NodeOffsets[v1]];
                        pre = tmp;
                        S.emplace_back(pre, v1);
                    }
                    v1 = pre;
                    if (ExistEdge(u1, u2, pend)) {
                        if (--degree[u1] == 2)
                            degreeTwos.push_back(u1);
                        if (--degree[u2] == 2)
                            degreeTwos.push_back(u2);
                        assert(degree[u1] > 1 && degree[u2] > 1);
                        for (auto k = m_NodeOffsets[u1]; k < pend[u1]; ++k)
                            if (is[m_Edges[k]] && !dominate[m_Edges[k]] && static_cast<int>(tri[k]) + 1 == degree[u1]) {
                                dominate[m_Edges[k]] = true;
                                dominated.push_back(m_Edges[k]);
                            }
                        for (auto k = m_NodeOffsets[u2]; k < pend[u2]; ++k)
                            if (is[m_Edges[k]] && !dominate[m_Edges[k]] && static_cast<int>(tri[k]) + 1 == degree[u2]) {
                                dominate[m_Edges[k]] = true;
                                dominated.push_back(m_Edges[k]);
                            }
                    } else {
                        EdgeRewrite(u1, pend, v1, u2);
                        EdgeRewrite(u2, pend, v2, u1);
                        UpdateTriangle(u1, u2, pend, is, adj, tri, degree, dominate, dominated);
                    }
                }
            }
        }
        if (firstTime) {
            firstTime = false;
            for (unsigned int k = 0; k < m_NodeCount; ++k)
                if (!is[k] || degree[k] <= 0)
                    fixed[k] = true;
        }
        while (dominated.empty() && degreeTwos.empty()) {
            while (maxD >= 3 && binHead[maxD] == -1)
                --maxD;
            if (maxD < 3)
                break;
            int v = -1;
            for (v = binHead[maxD]; v != -1;) {
                auto tmp = binNext[v];
                if (is[v] && degree[v] > 0) {
                    if (degree[v] < maxD) {
                        binNext[v] = binHead[degree[v]];
                        binHead[degree[v]] = v;
                    } else {
                        S.emplace_back(v, m_NodeCount);
                        DeleteVertex(v, pend, is, degreeTwos, tri, adj, degree, dominate, dominated);
                        binHead[maxD] = tmp;
                        break;
                    }
                }
                v = tmp;
            }
            if (v == -1)
                binHead[maxD] = -1;
        }
    }
    for (auto i = static_cast<int>(S.size()) - 1; i >= 0; --i) {
        auto u1 = S[i].first, u2 = S[i].second;
        assert(is[u1] == 0);
        if (u2 != m_NodeCount) {
            if (!is[u2])
                is[u1] = true;
            continue;
        }
        bool ok = true;
        for (auto j = m_NodeOffsets[u1]; j < m_NodeOffsets[u1 + 1]; ++j)
            if (is[m_Edges[j]]) {
                ok = false;
                break;
            }
        if (ok)
            is[u1] = true;
    }
    return is;
}
