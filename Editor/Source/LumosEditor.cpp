#include "Editor.h"
#include <Lumos/Core/EntryPoint.h>

Lumos::Application* Lumos::CreateApplication()
{
    return new Editor();
}
