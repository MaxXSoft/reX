#ifndef REX_RE_CHARSET_H_
#define REX_RE_CHARSET_H_

#include <memory>
#include <functional>
#include <unordered_set>
#include <limits>
#include <cassert>
#include <cstdint>

#include "util.h"

namespace rex {

class SymbolBase {
public:
    virtual ~SymbolBase() = default;

    virtual bool TestChar(char c) const = 0;
    virtual std::size_t GetHash() const = 0;

    virtual bool Equal(const SymbolBase *rhs) const {
        return rhs && rhs->type_ == type_;
    };

protected:
    enum class SymbolType : char {
        Char, Range, Set
    };

    SymbolBase(SymbolType type) : type_(type) {}

private:
    SymbolType type_;
};

using SymbolPtr = std::shared_ptr<SymbolBase>;

class CharSymbol : public SymbolBase {
public:
    CharSymbol(char c) : SymbolBase(SymbolType::Char), c_(c) {}

    bool TestChar(char c) const override { return c == c_; }
    std::size_t GetHash() const override { return std::hash<char>{}(c_); }

    bool Equal(const SymbolBase *rhs) const override {
        if (!SymbolBase::Equal(rhs)) return false;
        auto ptr = static_cast<const CharSymbol *>(rhs);
        return ptr->c_ == c_;
    }

private:
    char c_;
};

class RangeSymbol : public SymbolBase {
public:
    RangeSymbol(char c0, char c1)
            : SymbolBase(SymbolType::Range), c0_(c0), c1_(c1) {
        assert(c0 <= c1);
    }

    bool TestChar(char c) const override { return c0_ <= c && c <= c1_; }
    std::size_t GetHash() const override {
        auto hash_c0 = std::hash<char>{}(c0_);
        auto hash_c1 = std::hash<char>{}(c1_);
        HashCombile(hash_c0, hash_c1);
        return hash_c0;
    }

    bool Equal(const SymbolBase *rhs) const override {
        if (!SymbolBase::Equal(rhs)) return false;
        auto ptr = static_cast<const RangeSymbol *>(rhs);
        return ptr->c0_ == c0_ && ptr->c1_ == c1_;
    }

private:
    char c0_, c1_;
};

class SetSymbol : public SymbolBase {
public:
    SetSymbol(const std::uint64_t char_set[4])
            : SymbolBase(SymbolType::Set) {
        for (int i = 0; i < 4; ++i) char_set_[i] = char_set[i];
    }

    bool TestChar(char c) const override {
        auto index = static_cast<unsigned int>(c) & 0xff;
        return char_set_[index / 64] & (1ULL << (index % 64));
    }
    std::size_t GetHash() const override {
        std::size_t hash_val = 0;
        for (const auto &i : char_set_) {
            HashCombile(hash_val, std::hash<std::uint64_t>{}(i));
        }
        return hash_val;
    }

    bool Equal(const SymbolBase *rhs) const override {
        if (!SymbolBase::Equal(rhs)) return false;
        auto ptr = static_cast<const SetSymbol *>(rhs);
        for (int i = 0; i < 4; ++i) {
            if (ptr->char_set_[i] != char_set_[i]) return false;
        }
        return true;
    }

private:
    std::uint64_t char_set_[4];
};

struct SymbolHash {
    std::size_t operator()(const SymbolPtr &ptr) {
        return ptr->GetHash();
    }
};

struct SymbolEqual {
    bool operator()(const SymbolPtr &lhs, const SymbolPtr &rhs) {
        return lhs->Equal(rhs.get());
    }
};

using SymbolSet = std::unordered_set<SymbolPtr, SymbolHash, SymbolEqual>;

class CharSet {
public:
    using SymbolDef = std::function<bool(char)>;

    CharSet() { Clear(); }
    ~CharSet() {}

    class CharSetIterator {
    public:
        CharSetIterator(const std::uint64_t char_set[4], bool end) {
            index_ = -1;
            for (int i = 0; i < 4; ++i) {
                char_set_[i] = char_set[i];
                if (index_ == -1 && !end) {
                    for (int j = 0; j < 64; ++j) {
                        if ((char_set_[i] & (1ULL << j))) {
                            index_ = i * 64 + j;
                            break;
                        }
                    }
                }
            }
            if (end) {
                for (int j = 255; j >= 0; --j) {
                    if ((char_set_[j / 64] & (1ULL << j % 64))) {
                        index_ = j + 1;
                        break;
                    }
                }
            }
        }
        ~CharSetIterator() {}

        bool operator!=(const CharSetIterator &rhs) {
            return index_ != rhs.index_;
        }

        const CharSetIterator &operator++() {
            if (index_ < 256) {
                for (int i = index_ + 1; i < 256; ++i) {
                    if ((char_set_[i / 64] & (1ULL << i % 64))) {
                        index_ = i;
                        return *this;
                    }
                }
                ++index_;
            }
            return *this;
        }

        char operator*() const { return static_cast<char>(index_); }

    private:
        std::uint64_t char_set_[4];
        int index_;
    };

    void Insert(char c) {
        auto index = static_cast<unsigned int>(c) & 0xff;
        char_set_[index / 64] |= (1ULL << (index % 64));
    }

    void Remove(char c) {
        auto index = static_cast<unsigned int>(c) & 0xff;
        char_set_[index / 64] &= ~(1ULL << (index % 64));
    }

    void InsertSymbol(const SymbolPtr &symbol) {
        if (!symbol) return;
        auto char_min = std::numeric_limits<char>::min();
        auto char_max = std::numeric_limits<char>::max();
        for (int c = char_min; c <= char_max; ++c) {
            if (symbol->TestChar(c)) Insert(c);
        }
    }

    void InsertLambda(SymbolDef func) {
        auto char_min = std::numeric_limits<char>::min();
        auto char_max = std::numeric_limits<char>::max();
        for (int c = char_min; c <= char_max; ++c) {
            if (func(c)) Insert(c);
        }
    }

    SymbolPtr MakeSymbol() const {
        return Empty() ? nullptr : std::make_shared<SetSymbol>(char_set_);
    }

    bool Include(char c) const {
        auto index = static_cast<unsigned int>(c) & 0xff;
        return char_set_[index / 64] & (1ULL << (index % 64));
    }

    void Merge(const CharSet &char_set) {
        for (int i = 0; i < 4; ++i) char_set_[i] |= char_set.char_set_[i];
    }

    void Intersect(const CharSet &char_set) {
        for (int i = 0; i < 4; ++i) char_set_[i] &= char_set.char_set_[i];
    }

    void SymDiffer(const CharSet &char_set) {
        for (int i = 0; i < 4; ++i) char_set_[i] ^= char_set.char_set_[i];
    }

    void Reverse() {
        for (int i = 0; i < 4; ++i) char_set_[i] = ~char_set_[i];
    }

    void Clear() {
        for (auto &&i : char_set_) i = 0;
    }

    bool Empty() const {
        return !(char_set_[0] || char_set_[1]
                || char_set_[2] || char_set_[3]);
    }

    bool HasIntersection(const CharSet &rhs) {
        for (int i = 0; i < 4; ++i) {
            if ((char_set_[i] & rhs.char_set_[i])) return true;
        }
        return false;
    }

    CharSetIterator begin() const {
        return CharSetIterator(char_set_, false);
    }

    CharSetIterator end() const {
        return CharSetIterator(char_set_, true);
    }

    bool operator==(const CharSet &rhs) const {
        bool equal = true;
        for (int i = 0; i < 4; ++i) {
            equal &= char_set_[i] == rhs.char_set_[i];
        }
        return equal;
    }

    bool operator!=(const CharSet &rhs) const {
        for (int i = 0; i < 4; ++i) {
            if (char_set_[i] != rhs.char_set_[i]) return true;
        }
        return false;
    }

    operator bool() const {
        return !Empty();
    }

private:
    std::uint64_t char_set_[4];
};

} // namespace rex

#endif // REX_RE_CHARSET_H_
