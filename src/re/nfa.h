#ifndef REX_RE_NFA_H_
#define REX_RE_NFA_H_

#include <memory>
#include <utility>
#include <list>

#include "dfa.h"

namespace rex {

class NFAInterface {
public:
    virtual ~NFAInterface() = default;

    virtual DFAPtr GenerateDFA() = 0;
};

class NFAEdge;
class NFANode;
class NFAModel;

using NFAEdgePtr = std::shared_ptr<NFAEdge>;
using NFANodePtr = std::shared_ptr<NFANode>;
using NFAModelPtr = std::shared_ptr<NFAModel>;

class NFAEdge : public NFAInterface {
public:
    static const char kEmpty;

    NFAEdge(char c, const NFANodePtr &tail)
            : c_(c), tail_(tail) {}

    DFAPtr GenerateDFA() override;

    void set_tail(const NFANodePtr &tail) { tail_ = tail; }

private:
    char c_;
    NFANodePtr tail_;
};

class NFANode : public NFAInterface {
public:
    NFANode() {}

    void AddEdge(const NFAEdgePtr &edge) {
        out_edges_.push_back(edge);
    }

    DFAPtr GenerateDFA() override;

private:
    std::list<NFAEdgePtr> out_edges_;
};

class NFAModel : public NFAInterface {
public:
    NFAModel() {}

    void AddNode(const NFANodePtr &node) {
        nodes_.push_back(node);
    }

    void AddNodes(const std::list<NFANodePtr> &nodes) {
        for (const auto &node : nodes) nodes_.push_back(node);
    }

    DFAPtr GenerateDFA() override;

    void set_entry(const NFAEdgePtr &entry) {
        entry_ = entry;
    }

    void set_tail(const NFANodePtr &tail) {
        tail_ = tail;
    }

    const NFAEdgePtr &entry() const { return entry_; }
    const NFANodePtr &tail() const { return tail_; }
    const std::list<NFANodePtr> &nodes() const { return nodes_; }

private:
    NFAEdgePtr entry_;
    NFANodePtr tail_;
    std::list<NFANodePtr> nodes_;
};

} // namespace rex

#endif // REX_RE_NFA_H_
