#include "nfa.h"

#include <unordered_set>
#include <queue>
#include <iostream>
#include <iomanip>

namespace rex {

using NFANodeSet = std::unordered_set<NFANodePtr>;

const char NFAEdge::kEmpty = '\0';

NFANodeSet GetEpsilonClosure(const NFANodePtr &node) {
    auto node_set = NFANodeSet();
    auto node_queue = std::queue<NFANodePtr>();
    decltype(node_set.size()) last_size;
    node_queue.push(node);
    do {
        last_size = node_set.size();
        auto cur_node = node_queue.front();
        for (const auto &edge : cur_node->out_edges()) {
            if (edge->c() == NFAEdge::kEmpty) {
                node_queue.push(edge->tail());
            }
        }
        node_set.insert(cur_node);
        node_queue.pop();
    } while (node_set.size() != last_size);
    return node_set;
}

DFAPtr NFAEdge::GenerateDFA() {
    return nullptr;
}

DFAPtr NFANode::GenerateDFA() {
    return nullptr;
}

DFAPtr NFAModel::GenerateDFA() {
    return nullptr;
}

} // namespace rex
