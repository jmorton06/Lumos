#include "lmpch.h"
#include "GLIMGUIRenderer.h"
#include <imgui/imgui.h>
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <imgui/examples/imgui_impl_opengl3.h>

#include "GLDebug.h"

namespace Lumos
{
    namespace Graphics
    {
        GLIMGUIRenderer::GLIMGUIRenderer(u32 width, u32 height, bool clearScreen): m_WindowHandle(nullptr)
        {
	        m_ClearScreen = clearScreen;
	        ImGui_ImplOpenGL3_Init("#version 330");
            ImGui_ImplOpenGL3_NewFrame();
            
#ifdef LUMOS_PLATFORM_MACOS
            //ImGui::GetIO().DisplayFramebufferScale = {2.0f,2.0f};
#endif
        }

        GLIMGUIRenderer::~GLIMGUIRenderer()
        {
            ImGui_ImplOpenGL3_Shutdown();
        }

        void GLIMGUIRenderer::Init()
        {

        }

        void GLIMGUIRenderer::NewFrame()
        {

        }

        void GLIMGUIRenderer::Render(Lumos::Graphics::CommandBuffer* commandBuffer)
        {
			if (m_ClearScreen)
			{
				GLCall(glClear(GL_COLOR_BUFFER_BIT));
			}
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        void GLIMGUIRenderer::OnResize(u32 width, u32 height)
        {
            
        }

		void GLIMGUIRenderer::MakeDefault()
		{
			CreateFunc = CreateFuncGL;
		}

		IMGUIRenderer* GLIMGUIRenderer::CreateFuncGL(u32 width, u32 height, bool clearScreen)
		{
			return lmnew GLIMGUIRenderer(width, height, clearScreen);
		}
    }
}

