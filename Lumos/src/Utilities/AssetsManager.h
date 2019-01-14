#pragma once
#include "LM.h"
#include "Utilities/AssetManager.h"

namespace Lumos
{
	class Texture2D;
	class TextureCube;
	class Material;
	class Model;
	class Shader;
	class Object3D;

	class LUMOS_EXPORT AssetsManager
	{
	public:
		static AssetManager<Model>*		DefaultModels()   { return s_DefaultModels; };
		static AssetManager<Texture2D>* DefaultTextures() { return s_DefaultTextures; };

		static void InitializeMeshes();
		static void ReleaseMeshes();

	protected:
		static AssetManager<Model>*		s_DefaultModels;
		static AssetManager<Texture2D>* s_DefaultTextures;
	};
}
