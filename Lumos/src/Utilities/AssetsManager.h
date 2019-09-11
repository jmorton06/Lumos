#pragma once
#include "lmpch.h"
#include "Utilities/AssetManager.h"

namespace Lumos
{
	class Material;

	namespace Graphics
	{
		class Texture2D;
		class TextureCube;
		class Mesh;
		class Shader;
	}

	class LUMOS_EXPORT AssetsManager
	{
	public:
		static AssetManager<Graphics::Mesh>* DefaultModels() { return s_DefaultModels; };
		static AssetManager<Graphics::Texture2D>* DefaultTextures() { return s_DefaultTextures; };

		static void InitializeMeshes();
		static void ReleaseMeshes();

	protected:
		static AssetManager<Graphics::Mesh>* s_DefaultModels;
		static AssetManager<Graphics::Texture2D>* s_DefaultTextures;
	};
}
