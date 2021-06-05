#include <LumosEngine.h>
#include <Lumos/Core/EntryPoint.h>
#include "Editor.h"

Lumos::Application* Lumos::CreateApplication()
{
    return new Editor();
}
