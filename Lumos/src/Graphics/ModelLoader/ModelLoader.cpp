#include "lmpch.h"
#include "ModelLoader.h"
#include "Core/VFS.h"

namespace Lumos
{
	Entity* ModelLoader::LoadModel(const String& path)
	{
		std::string physicalPath;
		if (!Lumos::VFS::Get()->ResolvePhysicalPath(path, physicalPath))
		{
			LUMOS_LOG_INFO("Loaded Model - {0}", path);
			return nullptr;
		}

		String resolvedPath = physicalPath;

		const String fileExtension = StringFormat::GetFilePathExtension(path);

		if (fileExtension == "obj")
			return LoadOBJ(resolvedPath);
		else if (fileExtension == "gltf" || fileExtension == "glb")
			return LoadGLTF(resolvedPath);
		else
			LUMOS_LOG_CRITICAL("Unsupported File Type : {0}", fileExtension);

		LUMOS_LOG_INFO("Loaded Model - {0}", resolvedPath);

		return nullptr;
	}
}
