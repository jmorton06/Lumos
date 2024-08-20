#pragma once

namespace Lumos
{
    struct Version
    {
        int major = 0;
        int minor = 3;
        int patch = 9;
    };

    constexpr Version const LumosVersion = Version();
}
