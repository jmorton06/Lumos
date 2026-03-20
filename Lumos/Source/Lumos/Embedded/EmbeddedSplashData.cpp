#include "Precompiled.h"
#include "EmbeddedAssetData.h"
#include "splash.inl"

namespace Lumos
{
    namespace Embedded
    {
        const SplashData& GetSplashData()
        {
            static const SplashData data = {
                splashWidth,
                splashHeight,
                splash
            };
            return data;
        }
    }
}
