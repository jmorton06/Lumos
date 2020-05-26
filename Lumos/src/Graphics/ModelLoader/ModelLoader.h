#pragma once

#include "lmpch.h"

#include <entt/entt.hpp>

namespace Lumos
{
	namespace ModelLoader
	{
		LUMOS_EXPORT entt::entity LoadModel(const String& path, entt::registry& registry);
		entt::entity LoadOBJ(const String& path, entt::registry& registry);
		entt::entity LoadGLTF(const String& path, entt::registry& registry);
		entt::entity LoadFBX(const String& path, entt::registry& registry);
	};
}
