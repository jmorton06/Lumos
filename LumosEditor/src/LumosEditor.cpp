#include <LumosEngine.h>
#include <Core/EntryPoint.h>
#include "Editor.h"

Lumos::Application* Lumos::CreateApplication()
{
	return new Editor();
}
