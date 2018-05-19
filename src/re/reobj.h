#ifndef REX_RE_REOBJ_H_
#define REX_RE_REOBJ_H_

#include <string>
#include <utility>

#include "nfa.h"

namespace rex {

class REObject;

class RE {
public:
    static REObject Word(const std::string &word);
    static REObject Range(char c1, char c2);
    static REObject Nil();
};

class REObjectInterface {
public:
    virtual ~REObjectInterface() = default;
    virtual NFAPtr GenerateNFA() = 0;
};

class RENilObj : public REObjectInterface {
public:
    RENilObj() {}

    NFAPtr GenerateNFA() override;
};

class RECharObj : public REObjectInterface {
public:
    RECharObj(char c) : c_(c) {}

    NFAPtr GenerateNFA() override;

private:
    char c_;
};

class REAndObj : public REObjectInterface {
public:
    REAndObj(REObject lhs, REObject rhs)
            : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    NFAPtr GenerateNFA() override;

private:
    REObject lhs_, rhs_;
};

class REOrObj : public REObjectInterface {
public:
    REOrObj(REObject lhs, REObject rhs)
            : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    NFAPtr GenerateNFA() override;

private:
    REObject lhs_, rhs_;
};

class REKleeneObj : public REObjectInterface {
public:
    REKleeneObj(REObject reo) : reo_(std::move(reo)) {}

    NFAPtr GenerateNFA() override;

private:
    REObject reo_;
};

class REObject {
public:
    explicit REObject() : ptr_(nullptr), destruct_tag_(false) {}
    explicit REObject(REObjectInterface *ptr)
            : ptr_(ptr), destruct_tag_(true) {}
    REObject(const REObject &) = delete;
    REObject(REObject &&reo)
            : ptr_(reo.ptr_), destruct_tag_(reo.destruct_tag_) {
        reo.destruct_tag_ = false;
        reo.ptr_ = nullptr;
    }
    ~REObject() { Release(); }

    REObject &operator=(const REObject &) = delete;
    REObjectInterface *operator->() const noexcept { return ptr_; }
    explicit operator bool() const noexcept {
        return !destruct_tag_ || !ptr_;
    }

    REObject &operator=(REObject &&reo) noexcept {
        if (this != &reo) {
            Release();
            destruct_tag_ = reo.destruct_tag_;
            ptr_ = reo.ptr_;
            reo.destruct_tag_ = false;
            reo.ptr_ = nullptr;
        }
        return *this;
    }

    REObject &&operator&(REObject reo) {
        return REObject(new REAndObj(std::move(*this), std::move(reo)));
    }

    REObject &&operator|(REObject reo) {
        return REObject(new REOrObj(std::move(*this), std::move(reo)));
    }

    REObject &&Many() {
        return REObject(new REKleeneObj(std::move(*this)));
    }

    REObject &&Many1() {
        auto kleene = REObject(new REKleeneObj(std::move(*this)));
        return REObject(new REAndObj(std::move(*this), std::move(kleene)));
    }

    REObject &&Optional() {
        auto nil = RE::Nil();
        return REObject(new REOrObj(std::move(*this), std::move(nil)));
    }

    REObjectInterface *get() const { return ptr_; }

private:
    void Release() { if (destruct_tag_) delete ptr_; }

    REObjectInterface *ptr_;
    bool destruct_tag_;
};

} // namespace rex

#endif // REX_RE_REOBJ_H_
