#pragma once

namespace Lumos
{
    struct InternalVersion 
    {
        int major = 0;
        int minor = 2;
        int patch = 1;
    };

    constexpr InternalVersion const LumosVersion = {};

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

        int year = 2019;
        int release = 0;
        Stage stage = Stage::Preview;
        int rev = 2;
    };

constexpr Version const version = Version();
}
