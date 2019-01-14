#include "JM.h"
#include "GLIMGUIRenderer.h"
#include "external/imgui/imgui.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "external/imgui/examples/imgui_impl_opengl3.h"

namespace jm
{
    namespace graphics
    {
        GLIMGUIRenderer::GLIMGUIRenderer(uint width, uint height, void* windowHandle)
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

        void GLIMGUIRenderer::Render(jm::graphics::api::CommandBuffer* commandBuffer)
        {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
    }
}

