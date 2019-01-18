#include "LM.h"
#include "GLIMGUIRenderer.h"
#include "external/imgui/imgui.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "external/imgui/examples/imgui_impl_opengl3.h"

namespace Lumos
{
    namespace graphics
    {
        GLIMGUIRenderer::GLIMGUIRenderer(uint width, uint height)
        {
            m_Implemented = true;
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

        void GLIMGUIRenderer::Render(Lumos::graphics::api::CommandBuffer* commandBuffer)
        {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        void GLIMGUIRenderer::OnResize(uint width, uint height)
        {
            
        }
    }
}

