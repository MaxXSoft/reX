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

class SymbolInterface {
public:
    virtual ~SymbolInterface() = default;

    virtual bool TestChar(char c) const = 0;
};

using SymbolPtr = std::shared_ptr<SymbolInterface>;

class CharSymbol : public SymbolInterface {
public:
    CharSymbol(char c) : c_(c) {}

    bool TestChar(char c) const override { return c == c_; }

private:
    char c_;
};

class RangeSymbol : public SymbolInterface {
public:
    RangeSymbol(char c0, char c1) : c0_(c0), c1_(c1) { assert(c0 <= c1); }

    bool TestChar(char c) const override { return c0_ <= c && c <= c1_; }

private:
    char c0_, c1_;
};

class LambdaSymbol : public SymbolInterface {
public:
    using SymbolDef = std::function<bool(char)>;

    LambdaSymbol(SymbolDef func) : func_(func) {}

    bool TestChar(char c) const override { return func_(c); }

private:
    SymbolDef func_;
};

class CharSet {
public:
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
                        index_ = j;
                        break;
                    }
                }
            }
        }
        ~CharSetIterator() {}

        bool operator!=(const CharSetIterator &rhs) { index_ != rhs.index_; }

        const CharSetIterator &operator++() {
            if (index_ < 256) {
                for (int i = index_ + 1; i < 256; ++i) {
                    if ((char_set_[i / 64] & (1ULL << i % 64))) {
                        index_ = i;
                        break;
                    }
                }
            }
            return *this;
        }

        char operator*() const { return static_cast<char>(index_); }

    private:
        std::uint64_t char_set_[4];
        int index_;
    };

    void Insert(char c) {
        auto index = static_cast<unsigned int>(c);
        char_set_[index / 64] |= (1ULL << (index % 64));
    }

    void Remove(char c) {
        auto index = static_cast<unsigned int>(c);
        char_set_[index / 64] &= ~(1ULL << (index % 64));
    }

    void InsertSymbol(const SymbolPtr &symbol) {
        auto char_min = std::numeric_limits<char>::min();
        auto char_max = std::numeric_limits<char>::max();
        for (char c = char_min; c <= char_max; ++c) {
            if (symbol->TestChar(c)) Insert(c);
        }
    }

    SymbolPtr MakeSymbol() const {
        std::uint64_t cs[] = {
            char_set_[0], char_set_[1], char_set_[2], char_set_[3]
        };
        return std::make_shared<LambdaSymbol>([cs](char c) {
            auto index = static_cast<unsigned int>(c);
            return cs[index / 64] & (1ULL << (index % 64));
        });
    }

    bool Include(char c) const {
        auto index = static_cast<unsigned int>(c);
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
        return char_set_[0] || char_set_[1] || char_set_[2] || char_set_[3];
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

    friend bool HasIntersection(const CharSet &lhs, const CharSet &rhs);

private:
    std::uint64_t char_set_[4];
};

bool HasIntersection(const CharSet &lhs, const CharSet &rhs) {
    for (int i = 0; i < 4; ++i) {
        if ((lhs.char_set_[i] & rhs.char_set_[i])) return true;
    }
    return false;
}

} // namespace rex

#endif // REX_RE_CHARSET_H_
