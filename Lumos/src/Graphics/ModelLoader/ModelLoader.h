#pragma once

#include "lmpch.h"

#include <entt/entity/fwd.hpp>

namespace Lumos
{
	namespace ModelLoader
	{
		LUMOS_EXPORT entt::entity LoadModel(const std::string& path, entt::registry& registry);
		entt::entity LoadOBJ(const std::string& path, entt::registry& registry);
		entt::entity LoadGLTF(const std::string& path, entt::registry& registry);
		entt::entity LoadFBX(const std::string& path, entt::registry& registry);
	};
}
