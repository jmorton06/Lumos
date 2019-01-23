#pragma once

#include "LM.h"
#include "Maths/Maths.h"

namespace Lumos
{
	class Texture2D;
	class Shader;

	namespace graphics
	{
		namespace api
		{
			class Pipeline;
			class DescriptorSet;
			class UniformBuffer;
		}
	}
	struct MaterialProperties
	{
		float usingAlbedoMap 	= 1.0f;
		float usingSpecularMap 	= 1.0f;
		float usingGlossMap 	= 1.0f;
		float usingNormalMap 	= 1.0f;
		Lumos::maths::Vector4  albedoColour    = Lumos::maths::Vector4(1.0f,0.0f,1.0f,1.0f);
		Lumos::maths::Vector4  glossColour     = Lumos::maths::Vector4(1.0f,0.0f,1.0f,1.0f);
		Lumos::maths::Vector4  specularColour  = Lumos::maths::Vector4(0.0f,1.0f,0.0f,1.0f);
	};

	struct PBRMataterialTextures
	{
		std::shared_ptr<Texture2D> albedo;
		std::shared_ptr<Texture2D> normal;
		std::shared_ptr<Texture2D> metallic;
		std::shared_ptr<Texture2D> roughness;
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
		Material(std::shared_ptr<Shader>& shader, const MaterialProperties& properties = MaterialProperties(), const PBRMataterialTextures& textures = PBRMataterialTextures());
		Material();

		~Material();

		void SetRenderFlags(int flags) { m_RenderFlags = flags; }
		void SetRenderFlag(Material::RenderFlags flag) { m_RenderFlags |= static_cast<int>(flag); }
		void LoadPBRMaterial(const String& name, const String& path, const String& extension = ".png"); //TODO : Texture Parameters
		void LoadMaterial(const String& name, const String& path);
		void CreateDescriptorSet(graphics::api::Pipeline* pipeline, int layoutID);

		void SetTextures(const PBRMataterialTextures& textures);
        void SetMaterialProperites(const MaterialProperties& properties);
        void UpdateMaterialPropertiesData();

		PBRMataterialTextures 			GetTextures() 		const { return m_PBRMaterialTextures; }
		Shader* 						GetShader()			const { return m_Shader.get(); }
		graphics::api::DescriptorSet* 	GetDescriptorSet() 	const { return m_DescriptorSet; }
		int								GetRenderFlags()	const { return m_RenderFlags; }
		graphics::api::Pipeline* 		GetPipeline()		const { return m_Pipeline; }

	private:
		PBRMataterialTextures   		m_PBRMaterialTextures;
		std::shared_ptr<Shader> 		m_Shader;
		graphics::api::Pipeline* 		m_Pipeline;
		graphics::api::DescriptorSet* 	m_DescriptorSet;
		graphics::api::UniformBuffer* 	m_MaterialPropertiesBuffer;
		MaterialProperties*				m_MaterialProperties;
		uint							m_MaterialBufferSize;
        byte*                           m_MaterialBufferData;

	};
}
