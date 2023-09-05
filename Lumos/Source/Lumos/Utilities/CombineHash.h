#pragma once
#include <cstddef>

namespace Lumos
{
    inline void HashCombine(uint64_t& seed) { }

    template <typename T, typename... Rest>
    inline void HashCombine(uint64_t& seed, const T& v, Rest... rest)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        HashCombine(seed, rest...);
    }
}
