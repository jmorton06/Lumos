#pragma once

namespace Lumos
{
    struct InternalVersion
    {
        int major = 0;
        int minor = 3;
        int patch = 2;
    };

    constexpr InternalVersion const LumosVersion = InternalVersion();

    struct Version
    {
        enum class Stage
        {
            Preview,
            Alpha,
            Beta,
            RC,
            Release
        };

        int year = 2022;
        int release = 2;
        Stage stage = Stage::Alpha;
        int rev = 3;
    };

    constexpr Version const version = Version();
}
