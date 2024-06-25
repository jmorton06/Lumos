#include "Precompiled.h"
#include "DescriptorSet.h"

namespace Lumos
{
    namespace Graphics
    {
        DescriptorSet* (*DescriptorSet::CreateFunc)(const DescriptorDesc&) = nullptr;

        DescriptorSet* DescriptorSet::Create(const DescriptorDesc& desc)
        {
            LUMOS_ASSERT(CreateFunc, "No DescriptorSet Create Function");

            return CreateFunc(desc);
        }
    }
}
