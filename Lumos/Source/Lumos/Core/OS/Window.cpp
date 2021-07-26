#include "Precompiled.h"
#include "Window.h"

namespace Lumos
{
    Window* (*Window::CreateFunc)(const WindowDesc&) = NULL;

    Window* Window::Create(const WindowDesc& properties)
    {
        LUMOS_ASSERT(CreateFunc, "No Windows Create Function");
        return CreateFunc(properties);
    }

    bool Window::Initialise(const WindowDesc& properties)
    {
        return HasInitialised();
    }
}
