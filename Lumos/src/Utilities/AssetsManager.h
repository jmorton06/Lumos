#pragma once
#include "LM.h"
#include "Utilities/AssetManager.h"

namespace lumos
{
	class Material;

	namespace graphics
	{
		class Texture2D;
		class TextureCube;
		class Mesh;
		class Shader;
	}

	class LUMOS_EXPORT AssetsManager
	{
	public:
		static AssetManager<graphics::Mesh>*		DefaultModels()   { return s_DefaultModels; };
		static AssetManager<graphics::Texture2D>* DefaultTextures() { return s_DefaultTextures; };

		static void InitializeMeshes();
		static void ReleaseMeshes();

	protected:
		static AssetManager<graphics::Mesh>*		s_DefaultModels;
		static AssetManager<graphics::Texture2D>* s_DefaultTextures;
	};
}
