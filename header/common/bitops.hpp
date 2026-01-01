#pragma once

#include <bit>
#include <type_traits>

// helper alias (C++20+). For C++17, replace with remove_cv + remove_reference.
template <class T>
using rm_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

// 0/1 extractor (works even if `word` is an lvalue like DiffState[d.first])
#define GET_BIT(word, bit) \
    (((word) >> (bit)) & rm_cvref_t<decltype(word)>(1))

// type-correct masks (no more 1ULL forcing 64-bit)
#define SET_BIT(word, bit) \
    ((word) |= (rm_cvref_t<decltype(word)>(1) << (bit)))

#define UNSET_BIT(word, bit) \
    ((word) &= ~(rm_cvref_t<decltype(word)>(1) << (bit)))

#define TOGGLE_BIT(word, bit) \
    ((word) ^= (rm_cvref_t<decltype(word)>(1) << (bit)))

#define ROTATE_LEFT(x, n) std::rotl(x, n)
#define ROTATE_RIGHT(x, n) std::rotr(x, n)
