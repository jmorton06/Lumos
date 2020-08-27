#pragma once
#include "Core/Core.h"
#include "Graphics/API/VertexArray.h"

namespace Lumos
{
	namespace Graphics
	{
		class GLVertexArray : public VertexArray
		{
		private:
			u32 m_Handle;

		public:
			GLVertexArray();
			~GLVertexArray();

			_FORCE_INLINE_ VertexBuffer* GetBuffer(u32 index = 0) override;
			void PushBuffer(VertexBuffer* buffer) override;

			void Bind(CommandBuffer* commandBuffer) const override;
			void Unbind() const override;
            
            static void MakeDefault();
        protected:
            static VertexArray* CreateFuncGL();
		};
	}
}
