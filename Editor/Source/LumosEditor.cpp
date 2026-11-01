#include "Editor.h"
#define LUMOS_EXPORT_CLI
#include <Lumos/Core/EntryPoint.h>

Lumos::Application* Lumos::CreateApplication()
{
    return new Editor();
}
