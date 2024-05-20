#pragma once

namespace Lumos
{
    uint32_t MurmurHash3(const void* key, int len, uint32_t seed);
    uint64_t MurmurHash64A(const void* key, int len, uint64_t seed);
}
