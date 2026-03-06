#include "Precompiled.h"
#include "EmbeddedAssetData.h"
#include "BRDFTexture.inl"

namespace Lumos
{
    namespace Embedded
    {
        const BRDFData& GetBRDFData()
        {
            static const BRDFData data = {
                BRDFTextureWidth,
                BRDFTextureHeight,
                BRDFTexture
            };
            return data;
        }
    }
}
