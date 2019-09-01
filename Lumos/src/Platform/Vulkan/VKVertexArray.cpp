#include "LM.h"
#include "VKVertexArray.h"
#include "VKVertexBuffer.h"
#include "VKCommandBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		VKVertexArray::VKVertexArray()
		{
		}

		VKVertexArray::~VKVertexArray()
		{
			for (auto buffer : m_Buffers)
			{
				delete buffer;
			}
		}

		VertexBuffer* VKVertexArray::GetBuffer(u32 index)
		{
			return m_Buffers[index];
		}

		void VKVertexArray::PushBuffer(VertexBuffer* buffer)
		{
			m_Buffers.emplace_back(buffer);
			m_VKBuffers.emplace_back(dynamic_cast<VKVertexBuffer*>(buffer)->GetBuffer());
			m_Offsets.emplace_back(dynamic_cast<VKVertexBuffer*>(buffer)->GetLayout().GetStride());
		}

		void VKVertexArray::Bind(CommandBuffer* commandBuffer) const
		{
			static_cast<VKCommandBuffer*>(commandBuffer)->GetCommandBuffer().bindVertexBuffers(0, 1, m_VKBuffers.data(), m_Offsets.data());
		}

		void VKVertexArray::Unbind() const
		{

		}
        
        void VKVertexArray::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }
        
        VertexArray* VKVertexArray::CreateFuncVulkan()
        {
            return lmnew VKVertexArray();
        }
	}
}
