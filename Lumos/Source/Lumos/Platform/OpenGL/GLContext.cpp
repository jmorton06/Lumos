#include "Precompiled.h"
#include "GLContext.h"
#include "GLDebug.h"
#include "GL.h"

#include "Maths/Matrix4.h"
#include <imgui/imgui.h>

namespace Lumos
{
    namespace Graphics
    {

#ifdef GL_DEBUD_CALLBACK
        static std::string GetStringForType(GLenum type)
        {
            switch(type)
            {
            case GL_DEBUG_TYPE_ERROR:
                return "Error";
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                return "Deprecated behavior";
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                return "Undefined behavior";
            case GL_DEBUG_TYPE_PORTABILITY:
                return "Portability issue";
            case GL_DEBUG_TYPE_PERFORMANCE:
                return "Performance issue";
            case GL_DEBUG_TYPE_MARKER:
                return "Stream annotation";
            case GL_DEBUG_TYPE_OTHER_ARB:
                return "Other";
            default:
                return "";
            }
        }

        static bool PrintMessage(GLenum type)
        {
            switch(type)
            {
            case GL_DEBUG_TYPE_ERROR:
                return true;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                return true;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                return true;
            case GL_DEBUG_TYPE_PORTABILITY:
                return true;
            case GL_DEBUG_TYPE_PERFORMANCE:
                return false;
            case GL_DEBUG_TYPE_MARKER:
                return false;
            case GL_DEBUG_TYPE_OTHER_ARB:
                return false;
            default:
                return false;
            }
        }

        static std::string GetStringForSource(GLenum source)
        {
            switch(source)
            {
            case GL_DEBUG_SOURCE_API:
                return "API";
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                return "Window System";
            case GL_DEBUG_SOURCE_SHADER_COMPILER:
                return "Shader compiler";
            case GL_DEBUG_SOURCE_THIRD_PARTY:
                return "Third party";
            case GL_DEBUG_SOURCE_APPLICATION:
                return "Application";
            case GL_DEBUG_SOURCE_OTHER:
                return "Other";
            default:
                return "";
            }
        }

        namespace Lumos
        {
            static std::string GetStringForSeverity(GLenum severity)
            {
                switch(severity)
                {
                case GL_DEBUG_SEVERITY_HIGH:
                    ASSERT(0);
                    return "High";
                case GL_DEBUG_SEVERITY_MEDIUM:
                    ASSERT(0);
                    return "Medium";
                case GL_DEBUG_SEVERITY_LOW:
                    ASSERT(0);
                    return "Low";
                case GL_DEBUG_SEVERITY_NOTIFICATION:
                    return "Notification";
                case GL_DEBUG_SOURCE_API:
                    return "Source API";
                default:
                    return ("");
                }
            }

            void APIENTRY openglCallbackFunction(GLenum source,
                                                 GLenum type,
                                                 GLuint id,
                                                 GLenum severity,
                                                 GLsizei length,
                                                 const GLchar* message,
                                                 const void* userParam)
            {
                if(!PrintMessage(type))
                    return;

                LINFO(OPENGLLOG "Message: %s", message);
                LINFO(OPENGLLOG "Type: %s", GetStringForType(type).c_str());
                LINFO(OPENGLLOG "Source: %s", GetStringForSource(source).c_str());
                LINFO(OPENGLLOG "ID: %i", id);
                LINFO(OPENGLLOG "Severity: %s", GetStringForSeverity(source).c_str());
            }
        }
#endif
        GLContext::GLContext()
        {
            LINFO("----------------------------------");
            LINFO(OPENGLLOG);
            LINFO((const char*)(glGetString(GL_VERSION)));
            LINFO((const char*)(glGetString(GL_VENDOR)));
            LINFO((const char*)(glGetString(GL_RENDERER)));
            LINFO("----------------------------------");

#if LUMOS_DEBUG
#ifdef GL_DEBUD_CALLBACK
#ifndef LUMOS_PLATFORM_MACOS
            LINFO(OPENGLLOG "Registering OpenGL debug callback");

            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(Lumos::openglCallbackFunction, nullptr);
            GLuint unusedIds = 0;
            glDebugMessageControl(GL_DONT_CARE,
                                  GL_DONT_CARE,
                                  GL_DONT_CARE,
                                  0,
                                  &unusedIds,
                                  true);
#else
            LINFO(OPENGLLOG "glDebugMessageCallback not available");
#endif
#endif
#endif
            Maths::Matrix4::SetUpCoordSystem(false, false);
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
