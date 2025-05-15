#include "Precompiled.h"

#if defined(LUMOS_PLATFORM_MACOS)
#define GLFW_EXPOSE_NATIVE_COCOA
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GL.h"
#endif

#include "GLFWWindow.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Graphics/RHI/SwapChain.h"

#include "Utilities/LoadImage.h"

#include "GLFWKeyCodes.h"

#include "Core/OS/OS.h"
#include "Core/OS/Input.h"
#include "Core/Application.h"

#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <imgui/imgui.h>

static GLFWcursor* g_MouseCursors[ImGuiMouseCursor_COUNT] = { 0 };
#define LOG_CONTROLLER 0
namespace Lumos
{
    static bool s_GLFWInitialized = false;
    static int s_NumGLFWWindows   = 0;

    static void GLFWErrorCallback(int error, const char* description)
    {
        LERROR("GLFW Error - %i : %s", error, description);
    }

    GLFWWindow::GLFWWindow()
    {
        LUMOS_PROFILE_FUNCTION();
        m_Init  = false;
        m_VSync = false;

        m_HasResized = true;
        m_Init       = false;
    }

    GLFWWindow::~GLFWWindow()
    {
        for(ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
        {
            glfwDestroyCursor(g_MouseCursors[cursor_n]);
            g_MouseCursors[cursor_n] = NULL;
        }

        glfwDestroyWindow(m_Handle);

        s_NumGLFWWindows--;

        if(s_NumGLFWWindows < 1)
        {
            s_GLFWInitialized = false;
            glfwTerminate();
        }
    }

    bool GLFWWindow::Init(const WindowDesc& properties)
    {
        LUMOS_PROFILE_FUNCTION();
        LINFO("Creating window - Title : %s, Width : %i, Height : %i", properties.Title.str, properties.Width, properties.Height);

        m_VSync = properties.VSync;

        LINFO("VSync : %s", m_VSync ? "True" : "False");
        m_HasResized       = true;
        m_Data.m_RenderAPI = static_cast<Graphics::RenderAPI>(properties.RenderAPI);
        m_Data.VSync       = m_VSync;

        if(!s_GLFWInitialized)
        {
            int success = glfwInit();
            ASSERT(success, "Could not initialize GLFW!");
            glfwSetErrorCallback(GLFWErrorCallback);

            s_GLFWInitialized = true;
        }

        s_NumGLFWWindows++;

        float xscale = GetMonitorXScale();

        m_Data.DPIScale = xscale;

#ifdef LUMOS_PLATFORM_MACOS
        if(m_Data.DPIScale > 1.0f)
        {
            glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
            glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
        }
        else
        {
            glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
            glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
        }
#endif

#ifdef LUMOS_RENDER_API_OPENGL
        if(m_Data.m_RenderAPI == Graphics::RenderAPI::OPENGL)
        {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef LUMOS_PLATFORM_MACOS
            glfwWindowHint(GLFW_SAMPLES, 1);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
            glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
            glfwWindowHint(GLFW_COCOA_GRAPHICS_SWITCHING, GL_TRUE);
            glfwWindowHint(GLFW_STENCIL_BITS, 8); // Fixes 16 bit stencil bits in macOS.
            glfwWindowHint(GLFW_STEREO, GLFW_FALSE);
            glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
#endif
        }
#endif
        SetBorderlessWindow(properties.Borderless);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        uint32_t ScreenWidth  = 0;
        uint32_t ScreenHeight = 0;

        if(properties.Fullscreen)
        {
            ScreenWidth  = mode->width;
            ScreenHeight = mode->height;
        }
        else
        {
            ScreenWidth  = properties.Width;
            ScreenHeight = properties.Height;
        }

        m_Data.Title  = ToStdString(properties.Title);
        m_Data.Width  = ScreenWidth;
        m_Data.Height = ScreenHeight;
        m_Data.Exit   = false;

#ifdef LUMOS_RENDER_API_VULKAN
        if(m_Data.m_RenderAPI == Graphics::RenderAPI::VULKAN)
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

        m_Handle = glfwCreateWindow(ScreenWidth, ScreenHeight, (const char*)properties.Title.str, nullptr, nullptr);

        int w, h;
        glfwGetFramebufferSize(m_Handle, &w, &h);
        m_Data.Width  = w;
        m_Data.Height = h;

#ifdef LUMOS_RENDER_API_OPENGL
        if(m_Data.m_RenderAPI == Graphics::RenderAPI::OPENGL)
        {
            glfwMakeContextCurrent(m_Handle);

            if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            {
                LERROR("Failed to initialise OpenGL context");
            }
        }
#endif

        glfwSetWindowUserPointer(m_Handle, &m_Data);

        if(glfwRawMouseMotionSupported())
            glfwSetInputMode(m_Handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

        // #ifndef LUMOS_PLATFORM_MACOS
        SetIcon(properties);
        // #endif

        // glfwSetWindowPos(m_Handle, mode->width / 2 - m_Data.Width / 2, mode->height / 2 - m_Data.Height / 2);
        glfwSetInputMode(m_Handle, GLFW_STICKY_KEYS, true);

        // #ifdef LUMOS_PLATFORM_WINDOWS
        //		glfwGetWindowSize(m_Handle, &w, &h);
        //		m_Data.DPIScale = (float)w / m_Data.Width;
        // #endif

        // Set GLFW callbacks
        glfwSetWindowSizeCallback(m_Handle, [](GLFWwindow* window, int width, int height)
                                  {
                                      WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));

                                      int w, h;
                                      glfwGetFramebufferSize(window, &w, &h);

                                      data.DPIScale = (float)w / (float)width;

                                      data.Width = uint32_t(width * data.DPIScale);
                                      data.Height = uint32_t(height * data.DPIScale);

                                      WindowResizeEvent event(data.Width, data.Height, data.DPIScale);
                                      data.EventCallback(event); });

        glfwSetWindowCloseCallback(m_Handle, [](GLFWwindow* window)
                                   {
                                       WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));
                                       WindowCloseEvent event;
                                       data.EventCallback(event);
                                       data.Exit = true; });

        glfwSetWindowFocusCallback(m_Handle, [](GLFWwindow* window, int focused)
                                   {
                                       Window* lmWindow = Application::Get().GetWindow();

                                       if(lmWindow)
                                           lmWindow->SetWindowFocus(focused); });

        glfwSetWindowIconifyCallback(m_Handle, [](GLFWwindow* window, int32_t state)
                                     {
                                         switch(state)
                                         {
                                             case GL_TRUE:
                                             Application::Get().GetWindow()->SetWindowFocus(false);
                                             break;
                                             case GL_FALSE:
                                             Application::Get().GetWindow()->SetWindowFocus(true);
                                             break;
                                             default:
                                             LINFO("Unsupported window iconify state from callback");
                                         } });

        glfwSetKeyCallback(m_Handle, [](GLFWwindow* window, int key, int scancode, int action, int mods)
                           {

							   if(key < 0)
								   return;

                               WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));

                               switch(action)
                               {
                                   case GLFW_PRESS:
                                   {
                                       KeyPressedEvent event(GLFWKeyCodes::GLFWToLumosKeyboardKey(key), 0);
                                       data.EventCallback(event);
                                       break;
                                   }
                                   case GLFW_RELEASE:
                                   {
                                       KeyReleasedEvent event(GLFWKeyCodes::GLFWToLumosKeyboardKey(key));
                                       data.EventCallback(event);
                                       break;
                                   }
                                   case GLFW_REPEAT:
                                   {
                                       KeyPressedEvent event(GLFWKeyCodes::GLFWToLumosKeyboardKey(key), 1);
                                       data.EventCallback(event);
                                       break;
                                   }
                               } });

        glfwSetMouseButtonCallback(m_Handle, [](GLFWwindow* window, int button, int action, int mods)
                                   {
                                       WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));

                                       switch(action)
                                       {
                                           case GLFW_PRESS:
                                           {
                                               MouseButtonPressedEvent event(GLFWKeyCodes::GLFWToLumosMouseKey(button));
                                               data.EventCallback(event);
                                               break;
                                           }
                                           case GLFW_RELEASE:
                                           {
                                               MouseButtonReleasedEvent event(GLFWKeyCodes::GLFWToLumosMouseKey(button));
                                               data.EventCallback(event);
                                               break;
                                           }
                                       } });

        glfwSetScrollCallback(m_Handle, [](GLFWwindow* window, double xOffset, double yOffset)
                              {
                                  WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));
                                  MouseScrolledEvent event((float)xOffset, (float)yOffset);
                                  data.EventCallback(event); });

        glfwSetCursorPosCallback(m_Handle, [](GLFWwindow* window, double xPos, double yPos)
                                 {
                                     WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));
                                     MouseMovedEvent event((float)xPos /* * data.DPIScale*/, (float)yPos /* * data.DPIScale*/);
                                     data.EventCallback(event); });

        glfwSetCursorEnterCallback(m_Handle, [](GLFWwindow* window, int enter)
                                   {
                                       WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));

                                       MouseEnterEvent event(enter > 0);
                                       data.EventCallback(event); });

        glfwSetCharCallback(m_Handle, [](GLFWwindow* window, unsigned int keycode)
                            {
                                WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));

                                KeyTypedEvent event(GLFWKeyCodes::GLFWToLumosKeyboardKey(keycode), char(keycode));
                                data.EventCallback(event); });

        glfwSetDropCallback(m_Handle, [](GLFWwindow* window, int numDropped, const char** filenames)
                            {
                                WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));

                                std::string filePath = filenames[0];
                                WindowFileEvent event(filePath);
                                data.EventCallback(event); });

        g_MouseCursors[ImGuiMouseCursor_Arrow]      = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_TextInput]  = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeAll]  = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeNS]   = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeEW]   = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_Hand]       = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

        // Setting fullscreen overrides width and heigh in Init
        auto propCopy   = properties;
        propCopy.Width  = m_Data.Width;
        propCopy.Height = m_Data.Height;

        m_GraphicsContext = SharedPtr<Graphics::GraphicsContext>(Graphics::GraphicsContext::Create());
        m_GraphicsContext->Init();

        m_SwapChain = SharedPtr<Graphics::SwapChain>(Graphics::SwapChain::Create(m_Data.Width, m_Data.Height));
        m_SwapChain->Init(m_VSync, (Window*)this);

        m_Init = true;

        LINFO("Initialised GLFW version : %s", glfwGetVersionString());
        return true;
    }

    void GLFWWindow::SetIcon(const WindowDesc& desc)
    {
        TDArray<GLFWimage> images;

        if(!desc.IconData.Empty() && desc.IconData[0] != nullptr && desc.IconData.Size() == desc.IconDataSizes.Size())
        {
            for(int i = 0; i < desc.IconData.Size(); i++)
            {
                u8* data     = desc.IconData[i];
                u32 iconSize = desc.IconDataSizes[i];
                GLFWimage image;

                uint8_t* pixels = nullptr;

                image.height = iconSize;
                image.width  = iconSize;
                image.pixels = static_cast<unsigned char*>(data);
                images.PushBack(image);
            }

            glfwSetWindowIcon(m_Handle, int(images.Size()), images.Data());
        }
        else
        {
            for(auto& path : desc.IconPaths)
            {
                uint32_t width, height;
                GLFWimage image;

                uint8_t* pixels = nullptr;

                if(path.size)
                {
                    pixels = Lumos::LoadImageFromFile((const char*)path.str, &width, &height, nullptr, nullptr, false);

                    if(!pixels)
                    {
                        LWARN("Failed to load app icon %s", path.str);
                    }

                    image.height = height;
                    image.width  = width;
                    image.pixels = static_cast<unsigned char*>(pixels);
                    images.PushBack(image);
                }
            }

            glfwSetWindowIcon(m_Handle, int(images.Size()), images.Data());

            for(int i = 0; i < (int)images.Size(); i++)
            {
                delete[] images[i].pixels;
            }
        }
    }

    void GLFWWindow::SetWindowTitle(const std::string& title)
    {
        LUMOS_PROFILE_FUNCTION();
        glfwSetWindowTitle(m_Handle, title.c_str());
    }

    void GLFWWindow::ToggleVSync()
    {
        LUMOS_PROFILE_FUNCTION();
        if(m_VSync)
        {
            SetVSync(false);
        }
        else
        {
            SetVSync(true);
        }

        LINFO("VSync : %s", m_VSync ? "True" : "False");
    }

    void GLFWWindow::SetVSync(bool set)
    {
        LUMOS_PROFILE_FUNCTION();

        m_VSync = set;
        if(Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::OPENGL)
            glfwSwapInterval(set ? 1 : 0);

        LINFO("VSync : %s", m_VSync ? "True" : "False");
    }

    void GLFWWindow::OnUpdate()
    {
        LUMOS_PROFILE_FUNCTION();
#ifdef LUMOS_RENDER_API_OPENGL
        if(m_Data.m_RenderAPI == Graphics::RenderAPI::OPENGL)
        {
            LUMOS_PROFILE_SCOPE("GLFW SwapBuffers");
            glfwSwapBuffers(m_Handle);
        }
#endif
    }

    void GLFWWindow::SetBorderlessWindow(bool borderless)
    {
        LUMOS_PROFILE_FUNCTION();
        if(borderless)
        {
            glfwWindowHint(GLFW_DECORATED, false);
        }
        else
        {
            glfwWindowHint(GLFW_DECORATED, true);
        }
    }

    void GLFWWindow::HideMouse(bool hide)
    {
        LUMOS_PROFILE_FUNCTION();
        if(hide)
        {
            glfwSetInputMode(m_Handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else
        {
            glfwSetInputMode(m_Handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    void GLFWWindow::SetMousePosition(const Vec2& pos)
    {
        LUMOS_PROFILE_FUNCTION();
        Input::Get().StoreMousePosition(pos.x, pos.y);
        glfwSetCursorPos(m_Handle, pos.x, pos.y);
    }

    void GLFWWindow::MakeDefault()
    {
        CreateFunc = CreateFuncGLFW;
    }

    Window* GLFWWindow::CreateFuncGLFW()
    {
        return new GLFWWindow();
    }

    void GLFWWindow::UpdateCursorImGui()
    {
        LUMOS_PROFILE_FUNCTION();
        ImGuiIO& io                   = ImGui::GetIO();
        ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();

        if((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || glfwGetInputMode(m_Handle, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
            return;

        if(imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
        {
            // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor

            // TODO: This was disabled as it would override control of hiding the cursor
            //       Need to find a solution to support both
            // glfwSetInputMode(m_Handle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
        else
        {
            glfwSetCursor(m_Handle, g_MouseCursors[imgui_cursor] ? g_MouseCursors[imgui_cursor] : g_MouseCursors[ImGuiMouseCursor_Arrow]);
            // glfwSetInputMode(m_Handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    // #define LOG_CONTROLLER 0

    void GLFWWindow::ProcessInput()
    {
        LUMOS_PROFILE_FUNCTION();
        {
            LUMOS_PROFILE_SCOPE("GLFW PollEvents");
            glfwPollEvents();
        }
        {
            LUMOS_PROFILE_SCOPE("GLFW Update Controllers");
            UpdateControllers();
        }
    }

    void GLFWWindow::Maximise()
    {
        LUMOS_PROFILE_FUNCTION();
        glfwMaximizeWindow(m_Handle);

#ifdef LUMOS_PLATFORM_MACOS
        // TODO: Move to glfw extensions or something
        OS::Get().MaximiseWindow();
#endif
    }

    void GLFWWindow::UpdateControllers()
    {
        LUMOS_PROFILE_FUNCTION();
        auto& controllers = Input::Get().m_Controllers;

        // Update controllers
        for(int id = GLFW_JOYSTICK_1; id < GLFW_JOYSTICK_LAST; id++)
        {
            if(glfwJoystickPresent(id) == GLFW_TRUE)
            {
                Controller* controller = &controllers[id];
                if(!controller->Present)
                {
                    controller->ID      = id;
                    controller->Name    = glfwGetJoystickName(id);
                    controller->Present = true;
                }

                int buttonCount;
                const unsigned char* buttons = glfwGetJoystickButtons(id, &buttonCount);

                for(int i = 0; i < buttonCount; i++)
                {
                    if(buttons[i] == GLFW_PRESS && !controller->ButtonDown[i])
                        controller->ButtonStates[i].State = KeyState::Pressed;
                    else if(buttons[i] == GLFW_RELEASE && controller->ButtonDown[i])
                        controller->ButtonStates[i].State = KeyState::Released;

                    controller->ButtonDown[i] = buttons[i] == GLFW_PRESS;
                }

                int axisCount;
                const float* axes = glfwGetJoystickAxes(id, &axisCount);

                for(int i = 0; i < axisCount; i++)
                {
                    controller->AxisStates[i] = abs(axes[i]) > controller->DeadZones[i] ? axes[i] : 0.0f;
#if LOG_CONTROLLER
                    LINFO("State %i : %i", i, controller->AxisStates[i]);
#endif
                }

                int hatCount;
                const unsigned char* hats = glfwGetJoystickHats(id, &hatCount);
                for(int i = 0; i < hatCount; i++)
                    controller->HatStates[i] = hats[i];
            }
            else
                controllers[id].Present = false;
        }
    }

    float GLFWWindow::GetMonitorXScale()
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        float xscale, yscale;
        glfwGetMonitorContentScale(monitor, &xscale, &yscale);

        return xscale;
    }
}
