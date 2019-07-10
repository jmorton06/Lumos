#include "LM.h"
#include "GLIMGUIRenderer.h"
#include "external/imgui/imgui.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "external/imgui/examples/imgui_impl_opengl3.h"

#include "GLDebug.h"

namespace Lumos
{
    namespace Graphics
    {
        GLIMGUIRenderer::GLIMGUIRenderer(u32 width, u32 height, bool clearScreen): m_WindowHandle(nullptr)
        {
	        m_ClearScreen = clearScreen;
	        ImGui_ImplOpenGL3_Init("#version 410");
            ImGui_ImplOpenGL3_NewFrame();
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
    }
}

