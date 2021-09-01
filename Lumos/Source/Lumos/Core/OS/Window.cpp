#include "Precompiled.h"
#include "Window.h"

namespace Lumos
{
    Window* (*Window::CreateFunc)(const WindowDesc&) = NULL;

    Window::~Window()
    {
        m_SwapChain.reset();
        m_GraphicsContext.reset();
    }

    Window* Window::Create(const WindowDesc& windowDesc)
    {
        LUMOS_ASSERT(CreateFunc, "No Windows Create Function");
        return CreateFunc(windowDesc);
    }

    bool Window::Initialise(const WindowDesc& windowDesc)
    {
        return HasInitialised();
    }
}
