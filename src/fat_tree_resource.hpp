#pragma once

#include "fat_tree.hpp"
#include <optional>
#include <vector>

class FatTreeResource {
private:
    using Node = typename FatTree::Node;
    using Edge = typename FatTree::Edge;
    using AggrTree = typename FatTree::AggrTree;

    std::vector<unsigned int> m_NodeUsage;
    std::vector<unsigned int> m_EdgeUsage;

public:
    const FatTree *Topology;
    const std::optional<unsigned int> NodeQuota, LinkQuota;

    explicit FatTreeResource(const FatTree &topology, std::optional<unsigned int> nodeQuota,
                             std::optional<unsigned int> linkQuota);

    const std::vector<unsigned int> &GetNodeUsage() const { return m_NodeUsage; }
    const std::vector<unsigned int> &GetEdgeUsage() const { return m_EdgeUsage; }

    void Allocate(const AggrTree &tree);
    void Allocate(const std::vector<const Node *> &hosts);
    void Deallocate(const AggrTree &tree);
    void Deallocate(const std::vector<const Node *> &hosts);
    bool CheckTreeConflict(const AggrTree &tree) const;
    bool CheckTreeConflict(const AggrTree &tree1, const AggrTree &tree2) const;
};
