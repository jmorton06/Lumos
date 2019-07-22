#pragma once

#include "Graphics/API/VertexArray.h"

namespace vk
{
	class Buffer;
}

namespace Lumos
{
	namespace Graphics
	{
		class VKVertexArray : public VertexArray
		{

		public:
			VKVertexArray();
			~VKVertexArray();

			inline VertexBuffer* GetBuffer(u32 index = 0) override;
			void PushBuffer(VertexBuffer* buffer) override;

			void Bind(CommandBuffer* commandBuffer) const override;
			void Unbind() const override;

			const std::vector<vk::Buffer>& GetVKBuffers() const { return m_VKBuffers; }
			const std::vector<uint64_t>& GetOffsets() const { return m_Offsets; }

		private:
			std::vector<vk::Buffer> m_VKBuffers;
			std::vector<uint64_t> m_Offsets;
		};
	}
}
