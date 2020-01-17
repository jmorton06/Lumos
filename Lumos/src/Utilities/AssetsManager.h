#pragma once
#include "lmpch.h"
#include "Utilities/AssetManager.h"

namespace Lumos
{
	class Material;
	class Sound;

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
		static AssetManager<Sound>* Sounds() { return s_Sounds; };

		static void InitializeMeshes();
		static void ReleaseResources();

	protected:
		static AssetManager<Graphics::Mesh>* s_DefaultModels;
		static AssetManager<Graphics::Texture2D>* s_DefaultTextures;
		static AssetManager<Sound>* s_Sounds;
	};
}
