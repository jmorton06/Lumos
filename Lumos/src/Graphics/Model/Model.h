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
	struct Bone;

	class LUMOS_EXPORT Model
	{
	public:

		Model();
		explicit Model(const String& path, bool gamma = false);
		Model(const Model& other);
		explicit Model(const Mesh& mesh);
		explicit Model(const Mesh* mesh);
		explicit Model(std::shared_ptr<Mesh> mesh);
		virtual ~Model();

		virtual void Draw(bool bind = false);

		void SetMaterial(std::shared_ptr<Material> material);
		void SetMaterialFlag(Material::RenderFlags flag);
		void ResetRenderFlags();

		std::vector<std::shared_ptr<Mesh>> const & GetMeshs() const { return m_Meshes; }

		static std::shared_ptr<Texture2D> LoadMaterialTextures(const String& typeName, std::vector<std::shared_ptr<Texture2D>>& textures_loaded, const String& name, const String& directory, TextureParameters format);

		void SetNeedFrustumCheck(bool check) { m_NeedFrustumCheck = check; }
		bool GetNeedFrustumCheck() const { return m_NeedFrustumCheck; }

		Bone* GetRootBone() const { return m_RootBone; }

	protected:

		void  LoadModel(const String& path);
		void  LoadOBJ(const String& path);
		void  LoadGLTF(const String& path);

#ifdef ASSIMP
		void  ProcessNodeAssimp(aiNode* node, const aiScene* scene);
		Mesh* ProcessMeshAssimp(aiMesh* mesh, const aiScene* scene);
		void  LoadModelAssimp(const String& path);
#endif

		std::vector<std::shared_ptr<Mesh>> 		m_Meshes;
		std::vector<std::shared_ptr<Texture2D>> m_Textures;
		String					           		m_Directory;
		bool					           		m_GammaCorrection;
		bool                               		m_CleanUpMesh;
		bool							   		m_NeedFrustumCheck;
		Bone*							   		m_RootBone;
	};
}