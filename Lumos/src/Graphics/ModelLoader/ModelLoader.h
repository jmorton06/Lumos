#pragma once

#include "LM.h"

namespace lumos
{
	class Entity;

	namespace ModelLoader
	{
		LUMOS_EXPORT std::shared_ptr<Entity> LoadModel(const String& path);
		std::shared_ptr<Entity> LoadOBJ(const String& path);
		std::shared_ptr<Entity> LoadGLTF(const String& path);
	};
}
