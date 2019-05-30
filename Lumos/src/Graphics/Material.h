#pragma once

#include "LM.h"
#include "Maths/Maths.h"

namespace lumos
{
	namespace graphics
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
		lumos::maths::Vector4  albedoColour    = lumos::maths::Vector4(1.0f,0.0f,1.0f,1.0f);
		lumos::maths::Vector4  roughnessColour     = lumos::maths::Vector4(1.0f,0.0f,1.0f,1.0f);
		lumos::maths::Vector4  specularColour  = lumos::maths::Vector4(0.0f,1.0f,0.0f,1.0f);
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
		std::shared_ptr<graphics::Texture2D> albedo;
		std::shared_ptr<graphics::Texture2D> normal;
		std::shared_ptr<graphics::Texture2D> specular;
		std::shared_ptr<graphics::Texture2D> roughness;
		std::shared_ptr<graphics::Texture2D> ao;
		std::shared_ptr<graphics::Texture2D> emissive;
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
		Material(std::shared_ptr<graphics::Shader>& shader, const MaterialProperties& properties = MaterialProperties(), const PBRMataterialTextures& textures = PBRMataterialTextures());
		Material();
        
        Material(Material const&) = delete;
        Material& operator=(Material const&) = delete;

		~Material();

		void SetRenderFlags(int flags) { m_RenderFlags = flags; }
		void SetRenderFlag(Material::RenderFlags flag) { m_RenderFlags |= static_cast<int>(flag); }
		void LoadPBRMaterial(const String& name, const String& path, const String& extension = ".png"); //TODO : Texture Parameters
		void LoadMaterial(const String& name, const String& path);
		void CreateDescriptorSet(graphics::Pipeline* pipeline, int layoutID, bool pbr = true);

		void SetTextures(const PBRMataterialTextures& textures);
        void SetMaterialProperites(const MaterialProperties& properties);
        void UpdateMaterialPropertiesData();

		PBRMataterialTextures 			GetTextures() 		const { return m_PBRMaterialTextures; }
		graphics::Shader* 				GetShader()			const { return m_Shader.get(); }
		graphics::DescriptorSet* 		GetDescriptorSet() 	const { return m_DescriptorSet; }
		graphics::Pipeline* 			GetPipeline()		const { return m_Pipeline; }
		int								GetRenderFlags()	const { return m_RenderFlags; }
		String							GetName()			const { return m_Name; }
		MaterialProperties*				GetProperties()		const { return m_MaterialProperties; }

	private:
		PBRMataterialTextures   			m_PBRMaterialTextures;
		std::shared_ptr<graphics::Shader>	m_Shader;
		graphics::Pipeline* 				m_Pipeline;
		graphics::DescriptorSet* 			m_DescriptorSet;
		graphics::UniformBuffer* 			m_MaterialPropertiesBuffer;
		MaterialProperties*					m_MaterialProperties;
		uint								m_MaterialBufferSize;
        byte*								m_MaterialBufferData;
		String								m_Name;

	};
}
