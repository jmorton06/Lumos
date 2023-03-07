#include "Precompiled.h"
#include "GLIMGUIRenderer.h"
#include <imgui/imgui.h>
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
// #define IMGUI_IMPL_OPENGL_LOADER_GLAD
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
        }

        GLIMGUIRenderer::~GLIMGUIRenderer()
        {
            LUMOS_PROFILE_FUNCTION();
            ImGui_ImplOpenGL3_Shutdown();
        }

        void GLIMGUIRenderer::Init()
        {
            ImGui_ImplOpenGL3_Init("#version 410");
            ImGui_ImplOpenGL3_NewFrame();
        }

        void GLIMGUIRenderer::NewFrame()
        {
            LUMOS_PROFILE_FUNCTION();
            ImGui_ImplOpenGL3_NewFrame();
        }

        void GLIMGUIRenderer::Render(Lumos::Graphics::CommandBuffer* commandBuffer)
        {
            LUMOS_PROFILE_FUNCTION();
            ImGui::Render();

            if(m_ClearScreen)
            {
                GLCall(glClear(GL_COLOR_BUFFER_BIT));
            }
            // GLenum last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
            {
                LUMOS_PROFILE_SCOPE("GL3_Render");
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }
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
