#include "nfa.h"
#include "util.h"

#include <unordered_set>
#include <queue>
#include <unordered_map>

namespace {

using NFANodePtr = rex::NFANodePtr;
using SymbolPtr = rex::SymbolPtr;

class NFANodeSet : public std::unordered_set<NFANodePtr> {
public:
    using HashType = hasher::result_type;

    NFANodeSet() : std::unordered_set<NFANodePtr>(), hash_value_(0) {}

    void push(const NFANodePtr &ptr) {
        auto ret = insert(ptr);
        if (!ret.second) return;
        auto new_hash = hash_function()(ptr);
        rex::HashCombile(hash_value_, new_hash);
    }

    void merge(const NFANodeSet &node_set) {
        // TODO: optimize?
        for (const auto &i : node_set) insert(i);
        rex::HashCombile(hash_value_, node_set.hash_value_);
    }

    auto hash_value() const { return hash_value_; }

private:
    HashType hash_value_;
};

NFANodeSet GetEpsilonClosure(const NFANodePtr &node) {
    NFANodeSet node_set;
    std::queue<NFANodePtr> node_queue;
    decltype(node_set.size()) last_size;
    node_queue.push(node);
    do {
        last_size = node_set.size();
        const auto &cur_node = node_queue.front();
        for (const auto &edge : cur_node->out_edges()) {
            if (!edge->symbol()) {
                node_queue.push(edge->tail());
            }
        }
        node_set.push(cur_node);
        node_queue.pop();
    } while (node_set.size() != last_size && !node_queue.empty());
    return node_set;
}

// for DFA conversion
// TODO: optimize
NFANodeSet GetDFAState(const NFANodeSet &nodes, const SymbolPtr &symbol) {
    NFANodeSet node_set, final_set;
    for (const auto &node : nodes) {
        for (const auto &edge : node->out_edges()) {
            if (symbol->Equal(edge->symbol().get())) {
                node_set.push(edge->tail());
            }
        }
    }
    for (const auto &node : node_set) {
        auto ret = GetEpsilonClosure(node);
        final_set.merge(ret);
    }
    return final_set;
}

} // namespace

namespace rex {

void NFAModel::NormalizeNFA() {
    // add redundant epsilon edge for an entrance of NFA model
    if (entry_->symbol()) {
        auto nil_node = std::make_shared<NFANode>();
        auto nil_edge = std::make_shared<NFAEdge>(nullptr, nil_node);
        nil_node->AddEdge(entry_);
        entry_ = nil_edge;
    }
    // remove all unique symbol
    for (auto it = symbol_set_.begin(); it != symbol_set_.end(); ) {
        if ((*it).use_count() == 1) {
            it = symbol_set_.erase(it);
        }
        else {
            ++it;
        }
    }
}

// a rough implementation of subset construction
// TODO: optimize
DFAModelPtr NFAModel::GenerateDFA() {
    std::deque<NFANodeSet> set_queue;
    std::unordered_map<NFANodeSet::HashType, DFAStatePtr> state_set;
    auto model = std::make_shared<DFAModel>();
    // define 'Push' operation
    auto Push = [&set_queue, &state_set](const NFANodeSet &node_set) {
        // is empty set
        if (node_set.empty()) return state_set.end();
        // not unique
        auto it = state_set.find(node_set.hash_value());
        if (it != state_set.end()) return it;
        set_queue.push_back(node_set);
        // add new DFA state
        auto new_state = std::make_shared<DFAState>();
        auto ret = state_set.insert({node_set.hash_value(), new_state});
        return ret.first;
    };
    // normalization current NFA
    NormalizeNFA();
    // get initial states set & push into queue
    auto initial_set = GetEpsilonClosure(entry_->tail());
    auto it = Push(initial_set);
    // initialize DFA model
    model->set_initial(it->second);
    if (initial_set.find(tail_) != initial_set.end()) {
        model->AddFinalState(it->second);
    }
    else {
        model->AddState(it->second);
    }
    // traversal every unique DFA state
    while (!set_queue.empty()) {
        const auto &front = set_queue.front();
        const auto &cur_state = state_set[front.hash_value()];
        for (const auto &symbol : symbol_set_) {
            auto dfa_state = GetDFAState(front, symbol);
            auto it = Push(dfa_state);
            // empty state set (adding empty edge)
            if (it == state_set.end()) continue;
            // add edge to new state
            auto new_edge = std::make_shared<DFAEdge>(symbol, it->second);
            cur_state->AddEdge(new_edge);
            // current state is a final state of DFA
            if (dfa_state.find(tail_) != dfa_state.end()) {
                model->AddFinalState(it->second);
            }
            else {
                model->AddState(it->second);
            }
            // add symbol
            model->AddSymbol(symbol);
        }
        set_queue.pop_front();
    }
    return model;
}

} // namespace rex
