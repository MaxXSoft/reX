#ifndef REX_RE_DFA_H_
#define REX_RE_DFA_H_

#include <memory>
#include <utility>
#include <list>
#include <unordered_set>
#include <string>

#include "charset.h"

namespace rex {

class DFAEdge;
class DFAState;
class DFAModel;

using DFAEdgePtr = std::shared_ptr<DFAEdge>;
using DFAStatePtr = std::shared_ptr<DFAState>;
using DFAModelPtr = std::shared_ptr<DFAModel>;

class DFAEdge {
public:
    DFAEdge(const SymbolPtr &symbol, const DFAStatePtr &next)
            : symbol_(symbol), next_state_(next) {}
    ~DFAEdge() {}

    const SymbolPtr &symbol() const { return symbol_; }
    const DFAStatePtr &next_state() const { return next_state_; }

private:
    SymbolPtr symbol_;
    DFAStatePtr next_state_;
};

class DFAState {
public:
    DFAState() {}
    ~DFAState() {}

    void AddEdge(const DFAEdgePtr &edge) { out_edges_.push_back(edge); }
    void Release() { out_edges_.clear(); }

    const std::list<DFAEdgePtr> out_edges() const { return out_edges_; }

private:
    std::list<DFAEdgePtr> out_edges_;
};

class DFAModel {
public:
    DFAModel() {}
    ~DFAModel() { Release(); }

    void AddState(const DFAStatePtr &state) { states_.insert(state); }

    void AddFinalState(const DFAStatePtr &state) {
        final_states_.insert(state);
    }

    void AddSymbol(const SymbolPtr &symbol) { symbols_.insert(symbol); }

    void Simplify();
    bool TestString(const std::string &str);
    void GenerateStateTable();

#if NDEBUG
#else
    void Debug();
#endif

    void set_initial(const DFAStatePtr &state) { initial_ = state; }

private:
    using DFAStateSet = std::unordered_set<DFAStatePtr>;

    void Release(bool with_symbols = true) {
        initial_.reset();
        for (auto &&i : states_) i->Release();
        for (auto &&i : final_states_) i->Release();
        states_.clear();
        final_states_.clear();
        if (with_symbols) symbols_.clear();
    }

    DFAStatePtr initial_;
    DFAStateSet states_, final_states_;
    SymbolSet symbols_;
};

} // namespace rex

#endif // REX_RE_DFA_H_
