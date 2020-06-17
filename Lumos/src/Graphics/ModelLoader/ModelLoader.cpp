#include "lmpch.h"
#include "ModelLoader.h"
#include "Core/VFS.h"

#include <entt/entity/entity.hpp>

namespace Lumos
{
	entt::entity ModelLoader::LoadModel(const String& path, entt::registry& registry)
	{
		std::string physicalPath;
		if (!Lumos::VFS::Get()->ResolvePhysicalPath(path, physicalPath))
		{
			Debug::Log::Info("Loaded Model - {0}", path);
			return entt::null;
		}

		String resolvedPath = physicalPath;

		const String fileExtension = StringFormat::GetFilePathExtension(path);

		if (fileExtension == "obj")
			return LoadOBJ(resolvedPath, registry);
		else if (fileExtension == "gltf" || fileExtension == "glb")
			return LoadGLTF(resolvedPath, registry);
		else if (fileExtension == "fbx" || fileExtension == "FBX")
			return LoadFBX(resolvedPath, registry);
		else
			Debug::Log::Error("Unsupported File Type : {0}", fileExtension);

		Debug::Log::Info("Loaded Model - {0}", resolvedPath);

		return entt::null;
	}
}
