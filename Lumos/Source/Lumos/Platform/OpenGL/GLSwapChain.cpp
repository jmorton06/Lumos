#include "Precompiled.h"
#include "GLSwapChain.h"
#include "Graphics/RHI/Framebuffer.h"
#include "GLTexture.h"
#include "Core/OS/Window.h"
#include "GLDebug.h"
#ifndef LUMOS_PLATFORM_MOBILE
#include <GLFW/glfw3.h>
#endif

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
                    LUMOS_ASSERT(0, "");
                    return "High";
                case GL_DEBUG_SEVERITY_MEDIUM:
                    LUMOS_ASSERT(0, "");
                    return "Medium";
                case GL_DEBUG_SEVERITY_LOW:
                    LUMOS_ASSERT(0, "");
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

                LUMOS_LOG_INFO(OPENGLLOG "Message: {0}", message);
                LUMOS_LOG_INFO(OPENGLLOG "Type: {0}", GetStringForType(type));
                LUMOS_LOG_INFO(OPENGLLOG "Source: {0}", GetStringForSource(source));
                LUMOS_LOG_INFO(OPENGLLOG "ID: {0}", id);
                LUMOS_LOG_INFO(OPENGLLOG "Severity: {0}", GetStringForSeverity(source));
            }
        }
        #endif

    
        GLSwapChain::GLSwapChain(uint32_t width, uint32_t height)
        {
            FramebufferDesc info {};
            info.width = width;
            info.height = height;
            info.attachments = nullptr;
        }

        GLSwapChain::~GLSwapChain()
        {
            for(auto& buffer : swapChainBuffers)
                delete buffer;
        }

        bool GLSwapChain::Init(bool vsync)
        {
            LUMOS_LOG_INFO("----------------------------------");
            LUMOS_LOG_INFO(OPENGLLOG);
            LUMOS_LOG_INFO(glGetString(GL_VERSION));
            LUMOS_LOG_INFO(glGetString(GL_VENDOR));
            LUMOS_LOG_INFO(glGetString(GL_RENDERER));
            LUMOS_LOG_INFO("----------------------------------");

#if LUMOS_DEBUG
#ifdef GL_DEBUD_CALLBACK
#ifndef LUMOS_PLATFORM_MACOS
            LUMOS_LOG_INFO(OPENGLLOG "Registering OpenGL debug callback");

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
            LUMOS_LOG_INFO(OPENGLLOG "glDebugMessageCallback not available");
#endif
#endif
#endif
            
            return true;
        }

        Texture* GLSwapChain::GetCurrentImage()
        {
            return nullptr; //swapChainBuffers[0];
        }

        uint32_t GLSwapChain::GetCurrentBufferIndex() const
        {
            return 0;
        }

        size_t GLSwapChain::GetSwapChainBufferCount() const
        {
            return 1;
        }

        void GLSwapChain::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }

        SwapChain* GLSwapChain::CreateFuncGL(uint32_t width, uint32_t height)
        {
            return new GLSwapChain(width, height);
        }
    }
}
