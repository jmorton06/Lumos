#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "UUID.h"
#include "Maths/Random.h"

namespace Lumos
{
    UUID::UUID()
    {
        m_UUID = Random64::Rand(1, std::numeric_limits<uint64_t>::max());
    }

    UUID::UUID(uint64_t uuid)
        : m_UUID(uuid)
    {
    }

    UUID::UUID(const UUID& other)
        : m_UUID(other.m_UUID)
    {
    }
}
