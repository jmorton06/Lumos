#include "Precompiled.h"
#include "DescriptorSet.h"

namespace Lumos
{
	namespace Graphics
	{
        DescriptorSet*(*DescriptorSet::CreateFunc)(const DescriptorInfo&) = nullptr;

		DescriptorSet* DescriptorSet::Create(const DescriptorInfo& info)
		{
            LUMOS_ASSERT(CreateFunc, "No DescriptorSet Create Function");
            
            return CreateFunc(info);
		}
	}
}
