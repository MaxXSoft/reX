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

class NFAEdge {
public:
    NFAEdge(const SymbolPtr &symbol, const NFANodePtr &tail)
            : symbol_(symbol), tail_(tail) {}
    ~NFAEdge() {}

    void set_symbol(const SymbolPtr &symbol) { symbol_ = symbol; }
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

    void Release() { for (auto &&i : out_edges_) i.reset(); }

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
        symbol_set_.insert(symbol);
    }

    void AddSymbolSet(const SymbolSet &symbol_set) {
        for (const auto &symbol : symbol_set) {
            symbol_set_.insert(symbol);
        }
    }

    void Release() {
        entry_.reset();
        tail_.reset();
        for (auto &&i : nodes_) {
            i->Release();
            i.reset();
        }
    }

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
    const SymbolSet &symbol_set() const { return symbol_set_; }

private:
    void NormalizeNFA();

    NFAEdgePtr entry_;
    NFANodePtr tail_;
    std::list<NFANodePtr> nodes_;
    SymbolSet symbol_set_;
};

} // namespace rex

#endif // REX_RE_NFA_H_
