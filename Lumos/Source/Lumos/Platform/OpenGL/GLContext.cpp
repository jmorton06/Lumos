#include "Precompiled.h"
#include "GLContext.h"

#include "GL.h"

#include <glm/mat4x4.hpp>

#include <imgui/imgui.h>

namespace Lumos
{
    namespace Graphics
    {
        GLContext::GLContext()
        {
            //glm::mat4::SetUpCoordSystem(false, false);
        }

        GLContext::~GLContext() = default;

        void GLContext::Present()
        {
        }

        void GLContext::OnImGui()
        {
            ImGui::TextUnformatted("%s", (const char*)(glGetString(GL_VERSION)));
            ImGui::TextUnformatted("%s", (const char*)(glGetString(GL_VENDOR)));
            ImGui::TextUnformatted("%s", (const char*)(glGetString(GL_RENDERER)));
        }

        void GLContext::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }

        GraphicsContext* GLContext::CreateFuncGL()
        {
            return new GLContext();
        }
    }
}
