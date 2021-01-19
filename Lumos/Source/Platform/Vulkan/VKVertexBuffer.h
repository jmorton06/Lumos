#pragma once

#include "VKBuffer.h"
#include "Graphics/API/VertexBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		class VKVertexBuffer : public VertexBuffer, public VKBuffer
		{
		public:
			explicit VKVertexBuffer(const BufferUsage& usage);
			~VKVertexBuffer();

			void Resize(u32 size) override;
			void SetData(u32 size, const void* data) override;
			void SetDataSub(u32 size, const void* data, u32 offset) override;
			void ReleasePointer() override;

			void Bind(CommandBuffer* commandBuffer, Pipeline* pipeline) override;
			void Unbind() override;
            
            static void MakeDefault();
        protected:
            static VertexBuffer* CreateFuncVulkan(const BufferUsage& usage);
			void* GetPointerInternal() override;

			bool m_MappedBuffer = false;

			BufferUsage m_Usage;
			u32 m_Size;
		};

	}
}
