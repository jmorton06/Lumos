#include "lmpch.h"
#include "ModelLoader.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Core/OS/FileSystem.h"
//#include "Maths/BoundingSphere.h"
//#include "ECS/Component/MeshComponent.h"
//#include "ECS/Component/MaterialComponent.h"
//
//#include "Graphics/API/Texture.h"
//#include "Utilities/AssetsManager.h"
//#include "Maths/MathsUtilities.h"
//#include "Maths/Matrix4.h"
//#include "Maths/Transform.h"
//#include "App/Application.h"

#include <OpenFBX/src/ofbx.h>

namespace Lumos
{

	static Graphics::TextureWrap GetWrapMode(int mode)
	{
		switch (mode)
		{
		//case TINYGLTF_TEXTURE_WRAP_REPEAT: return Graphics::TextureWrap::REPEAT;
		//case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: return Graphics::TextureWrap::CLAMP_TO_EDGE;
		//case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return Graphics::TextureWrap::MIRRORED_REPEAT;
		default: return Graphics::TextureWrap::REPEAT;
		}
	}

	static Graphics::TextureFilter GetFilter(int value)
	{
		switch (value)
		{
		//case TINYGLTF_TEXTURE_FILTER_NEAREST:
		//case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		//case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
		//	return Graphics::TextureFilter::NEAREST;
		//case TINYGLTF_TEXTURE_FILTER_LINEAR:
		//case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
		//case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
		//	return Graphics::TextureFilter::LINEAR;
		//default: return Graphics::TextureFilter::LINEAR;
		}
	}

	entt::entity ModelLoader::LoadFBX(const String& path, entt::registry& registry)
	{
		std::string err;

		std::string ext = StringFormat::GetFilePathExtension(path);
		int size = FileSystem::GetFileSize(path);
		auto data = FileSystem::ReadFile(path);

		const bool ignoreGeometry = false;
		const u64 flags = ignoreGeometry ? (u64)ofbx::LoadFlags::IGNORE_GEOMETRY : (u64)ofbx::LoadFlags::TRIANGULATE;

		ofbx::IScene* scene = ofbx::load(data, size, flags);

		err = ofbx::getError();

		if (!err.empty() || !scene)
		{
			LUMOS_LOG_CRITICAL(err);
		}

		const ofbx::GlobalSettings* settings = scene->getGlobalSettings();

		String name = path.substr(path.find_last_of('/') + 1);

		auto entity = registry.create();

		return entity;
	}

}
