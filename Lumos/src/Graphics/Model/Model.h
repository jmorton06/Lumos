#pragma once

#include "LM.h"
#include "../Mesh.h"
#include "../Material.h"

#ifdef ASSIMP
struct aiMesh;
struct aiScene;
struct aiNode;
#endif

namespace Lumos
{
	class Shader;
	class Texture2D;
	struct TextureParameters;

	class LUMOS_EXPORT Model
	{
	public:

		Model();
		explicit Model(const String& path);
		Model(const Model& other);
		explicit Model(const Mesh& mesh);
		explicit Model(const Mesh* mesh);
		explicit Model(std::shared_ptr<Mesh> mesh);
		virtual ~Model();

		virtual void Draw(bool bind = false);

		void SetMaterial(std::shared_ptr<Material>& material);
		void SetMaterialFlag(Material::RenderFlags flag);
		void ResetRenderFlags();

		std::vector<std::shared_ptr<Mesh>> const & GetMeshs() const { return m_Meshes; }

		static std::shared_ptr<Texture2D> LoadMaterialTextures(const String& typeName, std::vector<std::shared_ptr<Texture2D>>& textures_loaded, const String& name, const String& directory, TextureParameters format);

	protected:

		void  LoadModel(const String& path);
		void  LoadOBJ(const String& path);
		void  LoadGLTF(const String& path);

		std::vector<std::shared_ptr<Mesh>> 		m_Meshes;
		std::vector<std::shared_ptr<Texture2D>> m_Textures;
		String					           		m_Directory;
		bool                               		m_CleanUpMesh;
	};
}
