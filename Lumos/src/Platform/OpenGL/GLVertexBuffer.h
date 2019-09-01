#pragma once

#include "Graphics/API/VertexBuffer.h"
#include "GLDebug.h"

namespace Lumos
{
	namespace Graphics
	{
		class GLVertexBuffer : public VertexBuffer
		{
		private:
			u32 m_Handle{};
			BufferUsage m_Usage;
			u32 m_Size;
			BufferLayout m_Layout;
		public:
			explicit GLVertexBuffer(BufferUsage usage);
			~GLVertexBuffer();

			void Resize(u32 size) override;
			void SetLayout(const BufferLayout& layout) override;
			void SetData(u32 size, const void* data) override;
			void SetDataSub(u32 size, const void* data, u32 offset) override;

			void ReleasePointer() override;

			void Bind() override;
			void Unbind() override;
            
            static void MakeDefault();
        protected:
            static CommandBuffer* CreateFuncGL();
            
		protected:
			void* GetPointerInternal() override;
		};
	}
}
