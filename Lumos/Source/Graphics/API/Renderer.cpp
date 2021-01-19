#include "Precompiled.h"
#include "Renderer.h"

namespace Lumos
{
	namespace Graphics
	{
        Renderer*(*Renderer::CreateFunc)(u32, u32) = nullptr;

		Renderer* Renderer::s_Instance = nullptr;

		void Renderer::Init(u32 width, u32 height)
		{
            LUMOS_ASSERT(CreateFunc, "No Renderer Create Function");
            
            s_Instance = CreateFunc(width, height);
			s_Instance->InitInternal();
		}

		void Renderer::Release()
		{
			delete s_Instance;

			s_Instance = nullptr;
		}
	}
}
