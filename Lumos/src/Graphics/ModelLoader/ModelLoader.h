#pragma once

#include "lmpch.h"

namespace Lumos
{
	class Entity;

	namespace ModelLoader
	{
		LUMOS_EXPORT Entity* LoadModel(const String& path);
		Entity* LoadOBJ(const String& path);
		Entity* LoadGLTF(const String& path);
	};
}
