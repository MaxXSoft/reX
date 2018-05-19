#ifndef REX_RE_REOBJ_H_
#define REX_RE_REOBJ_H_

#include <string>
#include <utility>

#include "nfa.h"

namespace rex {

class REObject;

class RE {
public:
    static REObject Nil();
    static REObject Word(const std::string &word);
    static REObject Range(char c1, char c2);
    static REObject And(REObject lhs, REObject rhs);
    static REObject Or(REObject lhs, REObject rhs);
    static REObject Many(REObject reo);
    static REObject Many1(REObject reo);
    static REObject Optional(REObject reo);
};

class REObjectInterface {
public:
    virtual ~REObjectInterface() = default;
    virtual NFAModelPtr GenerateNFA() = 0;
};

class REObject : public std::shared_ptr<REObjectInterface> {
public:
    explicit REObject()
            : std::shared_ptr<REObjectInterface>(nullptr) {}
    explicit REObject(REObjectInterface *ptr)
            : std::shared_ptr<REObjectInterface>(ptr) {}

    REObject operator&(REObject reo) {
        return RE::And(*this, reo);
    }

    REObject operator|(REObject reo) {
        return RE::Or(*this, reo);
    }

    REObject Many() {
        return RE::Many(*this);
    }

    REObject Many1() {
        return RE::Many1(*this);
    }

    REObject Optional() {
        return RE::Optional(*this);
    }
};

class RENilObj : public REObjectInterface {
public:
    RENilObj() {}

    NFAModelPtr GenerateNFA() override;
};

class RECharObj : public REObjectInterface {
public:
    RECharObj(char c) : c_(c) {}

    NFAModelPtr GenerateNFA() override;

private:
    char c_;
};

class REAndObj : public REObjectInterface {
public:
    REAndObj(REObject lhs, REObject rhs)
            : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    NFAModelPtr GenerateNFA() override;

private:
    REObject lhs_, rhs_;
};

class REOrObj : public REObjectInterface {
public:
    REOrObj(REObject lhs, REObject rhs)
            : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    NFAModelPtr GenerateNFA() override;

private:
    REObject lhs_, rhs_;
};

class REKleeneObj : public REObjectInterface {
public:
    REKleeneObj(REObject reo) : reo_(std::move(reo)) {}

    NFAModelPtr GenerateNFA() override;

private:
    REObject reo_;
};

} // namespace rex

#endif // REX_RE_REOBJ_H_
