#pragma once

namespace Lumos
{
    struct InternalVersion
    {
        int major = 0;
        int minor = 3;
        int patch = 1;
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

        int year = 2021;
        int release = 1;
        Stage stage = Stage::Alpha;
        int rev = 3;
    };

    constexpr Version const version = Version();
}
