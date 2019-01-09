#include "JM.h"
#include "GLIMGUIRenderer.h"
#include "App/Window.h"
#include "external/imgui/imgui.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "external/imgui/examples/imgui_impl_glfw.h"
#include "external/imgui/examples/imgui_impl_opengl3.h"

namespace jm
{
    namespace graphics
    {
        GLIMGUIRenderer::GLIMGUIRenderer(uint width, uint height, void* windowHandle)
        {
            m_Implemented = true;

            #if __APPLE__
                // GL 3.2 + GLSL 150
                const char* glsl_version = "#version 150";
            #else
                // GL 3.0 + GLSL 130
                const char* glsl_version = "#version 130";
            #endif

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)windowHandle, false);
            ImGui_ImplOpenGL3_Init(glsl_version);

            // Setup style
            ImGui::StyleColorsDark();
        }

        GLIMGUIRenderer::~GLIMGUIRenderer()
        {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        }

        void GLIMGUIRenderer::Init()
        {

        }

        void GLIMGUIRenderer::NewFrame()
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }

        void GLIMGUIRenderer::Render(jm::graphics::api::CommandBuffer* commandBuffer)
        {
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
    }
}

