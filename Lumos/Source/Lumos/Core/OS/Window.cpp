#include "Precompiled.h"
#include "Window.h"

namespace Lumos
{
    Window* (*Window::CreateFunc)() = NULL;

    Window::~Window()
    {
        m_SwapChain.reset();
        m_GraphicsContext.reset();
    }

    Window* Window::Create()
    {
        ASSERT(CreateFunc, "No Windows Create Function");
        return CreateFunc();
    }
}
