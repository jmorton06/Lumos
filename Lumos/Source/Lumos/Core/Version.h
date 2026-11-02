#pragma once

namespace Lumos
{
    struct Version
    {
        int major = 0;
        int minor = 5;
        int patch = 0;
    };

    constexpr Version const LumosVersion = Version();
}
