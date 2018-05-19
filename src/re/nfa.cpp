#include "nfa.h"

#include <unordered_set>
#include <iostream>
#include <iomanip>

namespace rex {

using NFANodeSet = std::unordered_set<NFANodePtr>;

const char NFAEdge::kEmpty = '\0';

NFANodeSet GetEpsilonClosure(const NFANodePtr &node) {
    auto cur_state = static_cast<NFANode *>(node.get());
    auto node_set = NFANodeSet();
    node_set.insert(node);
    //
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
