#include "LM.h"
#include "ModelLoader.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "System/VFS.h"
#include "../Material.h"
#include "App/SceneManager.h"
#include "../Mesh.h"
#include "../API/Shader.h"
#include "Utilities/AssetsManager.h"

namespace Lumos
{
	std::shared_ptr<Entity> ModelLoader::LoadModel(const String& path)
	{
		std::string physicalPath;
		Lumos::VFS::Get()->ResolvePhysicalPath(path, physicalPath);

		String resolvedPath = physicalPath;

		const String fileExtension = StringFormat::GetFilePathExtension(path);

		if (fileExtension == "obj")
			return LoadOBJ(resolvedPath);
		else if (fileExtension == "gltf" || fileExtension == "glb")
			return LoadGLTF(resolvedPath);
		else
			LUMOS_CORE_ERROR("Unsupported File Type : {0}", fileExtension);

		LUMOS_CORE_INFO("Loaded Model - {0}", resolvedPath);

		return nullptr;
	}
}
