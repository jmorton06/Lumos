#pragma once

#include "LM.h"
#include "Maths/Maths.h"

namespace Lumos
{
	namespace Graphics
	{
		class Texture2D;
		class Shader;
		class Pipeline;
		class DescriptorSet;
		class UniformBuffer;
	}

	const int PBR_WORKFLOW_SEPARATE_TEXTURES  = 0;
	const int PBR_WORKFLOW_METALLIC_ROUGHNESS = 1;
	const int PBR_WORKFLOW_SPECULAR_GLOSINESS = 2;

	struct MaterialProperties
	{
		Maths::Vector4  albedoColour    = Maths::Vector4(1.0f,0.0f,1.0f,1.0f);
		Maths::Vector4  roughnessColour = Maths::Vector4(1.0f,0.0f,1.0f,1.0f);
		Maths::Vector4  specularColour  = Maths::Vector4(0.0f,1.0f,0.0f,1.0f);
		float usingAlbedoMap = 1.0f;
		float usingSpecularMap = 1.0f;
		float usingRoughnessMap = 1.0f;
		float usingNormalMap = 1.0f;
		float usingAOMap = 1.0f;
		float usingEmissiveMap = 1.0f;
		int workflow = PBR_WORKFLOW_SEPARATE_TEXTURES;
		float p1 = 1.0f;
	};

	struct PBRMataterialTextures
	{
		std::shared_ptr<Graphics::Texture2D> albedo;
		std::shared_ptr<Graphics::Texture2D> normal;
		std::shared_ptr<Graphics::Texture2D> specular;
		std::shared_ptr<Graphics::Texture2D> roughness;
		std::shared_ptr<Graphics::Texture2D> ao;
		std::shared_ptr<Graphics::Texture2D> emissive;
	};

	class LUMOS_EXPORT Material
	{
	public:
		enum class RenderFlags
		{
			NONE = 0,
			DISABLE_DEPTH_TEST = BIT(0),
			WIREFRAME = BIT(1),
			FORWARDRENDER = BIT(2),
			DEFERREDRENDER = BIT(3),
			NOSHADOW = BIT(4),
		};
	protected:
		int m_RenderFlags;
	public:
		Material(std::shared_ptr<Graphics::Shader>& shader, const MaterialProperties& properties = MaterialProperties(), const PBRMataterialTextures& textures = PBRMataterialTextures());
		Material();
        
        Material(Material const&) = delete;
        Material& operator=(Material const&) = delete;

		~Material();

		void SetRenderFlags(int flags) { m_RenderFlags = flags; }
		void SetRenderFlag(Material::RenderFlags flag) { m_RenderFlags |= static_cast<int>(flag); }
		void LoadPBRMaterial(const String& name, const String& path, const String& extension = ".png"); //TODO : Texture Parameters
		void LoadMaterial(const String& name, const String& path);
		void CreateDescriptorSet(Graphics::Pipeline* pipeline, int layoutID, bool pbr = true);

		void SetTextures(const PBRMataterialTextures& textures);
        void SetMaterialProperites(const MaterialProperties& properties);
        void UpdateMaterialPropertiesData();

		PBRMataterialTextures 			GetTextures() 		const { return m_PBRMaterialTextures; }
		Graphics::Shader* 				GetShader()			const { return m_Shader.get(); }
		Graphics::DescriptorSet* 		GetDescriptorSet() 	const { return m_DescriptorSet; }
		Graphics::Pipeline* 			GetPipeline()		const { return m_Pipeline; }
		int								GetRenderFlags()	const { return m_RenderFlags; }
		String							GetName()			const { return m_Name; }
		MaterialProperties*				GetProperties()		const { return m_MaterialProperties; }

		void OnImGui();

	private:
		PBRMataterialTextures   			m_PBRMaterialTextures;
		std::shared_ptr<Graphics::Shader>	m_Shader;
		Graphics::Pipeline* 				m_Pipeline;
		Graphics::DescriptorSet* 			m_DescriptorSet;
		Graphics::UniformBuffer* 			m_MaterialPropertiesBuffer;
		MaterialProperties*					m_MaterialProperties;
		u32									m_MaterialBufferSize;
        u8*									m_MaterialBufferData;
		String								m_Name;

	};
}
