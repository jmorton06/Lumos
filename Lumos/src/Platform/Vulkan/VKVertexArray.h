#pragma once
#include "VK.h"
#include "Graphics/API/VertexArray.h"

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

			const std::vector<VkBuffer>& GetVKBuffers() const { return m_VKBuffers; }
			const std::vector<uint64_t>& GetOffsets() const { return m_Offsets; }
            
            static void MakeDefault();
        protected:
            static VertexArray* CreateFuncVulkan();
		private:
			std::vector<VkBuffer> m_VKBuffers;
			std::vector<uint64_t> m_Offsets;
		};
	}
}
