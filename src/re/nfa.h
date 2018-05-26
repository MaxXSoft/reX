#ifndef REX_RE_NFA_H_
#define REX_RE_NFA_H_

#include <memory>
#include <utility>
#include <list>
#include <unordered_set>

#include "charset.h"
#include "dfa.h"

namespace rex {

class NFAEdge;
class NFANode;
class NFAModel;

using NFAEdgePtr = std::shared_ptr<NFAEdge>;
using NFANodePtr = std::shared_ptr<NFANode>;
using NFAModelPtr = std::shared_ptr<NFAModel>;

/*
    TODO:
        SymbolSet in NFA model
        optimize 'or' login
        compress the states in state table generating process
*/
// using SymbolSet = 

class NFAEdge {
public:
    NFAEdge(const SymbolPtr &symbol, const NFANodePtr &tail)
            : symbol_(symbol), tail_(tail) {}
    ~NFAEdge() {}

    const SymbolPtr &symbol() const { return symbol_; }
    const NFANodePtr &tail() const { return tail_; }

private:
    SymbolPtr symbol_;
    NFANodePtr tail_;
};

class NFANode {
public:
    NFANode() {}
    ~NFANode() {}

    void AddEdge(const NFAEdgePtr &edge) { out_edges_.push_back(edge); }

    const std::list<NFAEdgePtr> &out_edges() const { return out_edges_; }

private:
    std::list<NFAEdgePtr> out_edges_;
};

class NFAModel {
public:
    NFAModel() {}
    ~NFAModel() {}

    void AddNode(const NFANodePtr &node) {
        nodes_.push_back(node);
    }

    void AddNodes(const std::list<NFANodePtr> &nodes) {
        for (const auto &node : nodes) nodes_.push_back(node);
    }

    void AddSymbol(const SymbolPtr &symbol) {
        char_set_.InsertSymbol(symbol);
    }

    void AddCharSet(const CharSet &char_set) { char_set_.Merge(char_set); }

    void Release() { for (auto &&i : nodes_) i.reset(); }

    DFAModelPtr GenerateDFA();

    void set_entry(const NFAEdgePtr &entry) {
        entry_ = entry;
    }

    void set_tail(const NFANodePtr &tail) {
        tail_ = tail;
    }

    const NFAEdgePtr &entry() const { return entry_; }
    const NFANodePtr &tail() const { return tail_; }
    const std::list<NFANodePtr> &nodes() const { return nodes_; }
    const CharSet &char_set() const { return char_set_; }

private:
    NFAEdgePtr entry_;
    NFANodePtr tail_;
    std::list<NFANodePtr> nodes_;
    CharSet char_set_;
};

} // namespace rex

#endif // REX_RE_NFA_H_
