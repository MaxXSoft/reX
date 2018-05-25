#ifndef REX_RE_NFA_H_
#define REX_RE_NFA_H_

#include <memory>
#include <utility>
#include <list>
#include <unordered_set>

#include "dfa.h"

namespace rex {

class NFAEdge;
class NFANode;
class NFAModel;

using NFAEdgePtr = std::shared_ptr<NFAEdge>;
using NFANodePtr = std::shared_ptr<NFANode>;
using NFAModelPtr = std::shared_ptr<NFAModel>;
using CharSet = std::unordered_set<char>;

class NFAEdge {
public:
    static const char kEmpty;

    NFAEdge(char c, const NFANodePtr &tail)
            : c_(c), tail_(tail) {}
    ~NFAEdge() {}

    char c() const { return c_; }
    const NFANodePtr &tail() const { return tail_; }

private:
    char c_;
    NFANodePtr tail_;
};

class NFANode {
public:
    NFANode() {}
    ~NFANode() {}

    void AddEdge(const NFAEdgePtr &edge) {
        out_edges_.push_back(edge);
    }

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

    void AddChar(char c) { char_set_.insert(c); }
    void AddCharSet(const CharSet &char_set) {
        for (const auto &c : char_set) char_set_.insert(c);
    }

    void Release() { for (auto &&i : nodes_) i.reset(); }

    DFAPtr GenerateDFA();

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
