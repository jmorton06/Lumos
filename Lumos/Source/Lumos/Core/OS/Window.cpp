#include "Precompiled.h"
#include "Window.h"

namespace Lumos
{
    Window* (*Window::CreateFunc)(const WindowProperties&) = NULL;

    Window* Window::Create(const WindowProperties& properties)
    {
        LUMOS_ASSERT(CreateFunc, "No Windows Create Function");
        return CreateFunc(properties);
    }

    bool Window::Initialise(const WindowProperties& properties)
    {
        return HasInitialised();
    }
}
