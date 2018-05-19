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

using NFAPtr = std::shared_ptr<NFAInterface>;
using NFAPtrList = std::list<NFAPtr>;

class NFAEdge : public NFAInterface {
public:
    static const char kEmpty = '\0';

    NFAEdge(char c, const NFAPtr &tail)
            : c_(c), tail_(tail) {}

    DFAPtr GenerateDFA() override;

    void set_tail(const NFAPtr &tail) { tail_ = tail; }

private:
    char c_;
    NFAPtr tail_;
};

class NFANode : public NFAInterface {
public:
    NFANode() {}

    void AddEdge(const NFAPtr &edge) {
        out_edges_.push_back(edge);
    }

    DFAPtr GenerateDFA() override;

private:
    NFAPtrList out_edges_;
};

class NFAModel : public NFAInterface {
public:
    NFAModel() {}

    void AddNode(const NFAPtr &node) {
        nodes_.push_back(node);
    }

    void AddNodes(const NFAPtrList &nodes) {
        for (const auto &node : nodes) nodes_.push_back(node);
    }

    DFAPtr GenerateDFA() override;

    void set_entry(const std::shared_ptr<NFAEdge> &entry) {
        entry_ = entry;
    }

    void set_tail(const std::shared_ptr<NFANode> &tail) {
        tail_ = tail;
    }

    const std::shared_ptr<NFAEdge> &entry() const { return entry_; }
    const std::shared_ptr<NFANode> &tail() const { return tail_; }
    const NFAPtrList &nodes() const { return nodes_; }

private:
    std::shared_ptr<NFAEdge> entry_;
    std::shared_ptr<NFANode> tail_;
    NFAPtrList nodes_;
};

} // namespace rex

#endif // REX_RE_NFA_H_
