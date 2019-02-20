#ifndef REX_RE_UTIL_UTIL_H_
#define REX_RE_UTIL_UTIL_H_

#include <cstddef>

namespace rex::re {

inline void HashCombile(std::size_t &hash_value, std::size_t new_hash) {
    const auto magic_num = 0xc6a4a7935bd1e995ULL;
    const int offset = 47;
    new_hash *= magic_num;
    new_hash ^= new_hash >> offset;
    new_hash *= magic_num;
    hash_value ^= new_hash;
    hash_value *= magic_num;
}

} // namespace rex::re

#endif // REX_RE_UTIL_UTIL_H_
