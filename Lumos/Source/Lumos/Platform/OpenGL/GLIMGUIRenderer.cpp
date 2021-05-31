#include "Precompiled.h"
#include "GLIMGUIRenderer.h"
#include <imgui/imgui.h>
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <imgui/backends/imgui_impl_opengl3.h>

#include "GLDebug.h"

namespace Lumos
{
    namespace Graphics
    {
        GLIMGUIRenderer::GLIMGUIRenderer(uint32_t width, uint32_t height, bool clearScreen)
            : m_WindowHandle(nullptr)
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
            ImGui::Render();

            if(m_ClearScreen)
            {
                GLCall(glClear(GL_COLOR_BUFFER_BIT));
            }
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        void GLIMGUIRenderer::OnResize(uint32_t width, uint32_t height)
        {
        }

        void GLIMGUIRenderer::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }

        IMGUIRenderer* GLIMGUIRenderer::CreateFuncGL(uint32_t width, uint32_t height, bool clearScreen)
        {
            return new GLIMGUIRenderer(width, height, clearScreen);
        }

        void GLIMGUIRenderer::RebuildFontTexture()
        {
            ImGui_ImplOpenGL3_DestroyFontsTexture();
            ImGui_ImplOpenGL3_CreateFontsTexture();
        }
    }
}
