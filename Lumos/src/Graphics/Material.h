#pragma once

#include "lmpch.h"
#include "Maths/Maths.h"
#include "Graphics/API/Shader.h"
#include "Graphics/API/Texture.h"

namespace Lumos
{
	namespace Graphics
	{
		class Pipeline;
		class DescriptorSet;
		class UniformBuffer;
	}

const float PBR_WORKFLOW_SEPARATE_TEXTURES  = 0.0f;
const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 1.0f;
const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 2.0f;

	struct MaterialProperties
	{
		Maths::Vector4  albedoColour    = Maths::Vector4(1.0f,0.0f,1.0f,1.0f);
		Maths::Vector4  roughnessColour = Maths::Vector4(1.0f,0.0f,1.0f,1.0f);
		Maths::Vector4  metallicColour  = Maths::Vector4(0.0f,1.0f,0.0f,1.0f);
        Maths::Vector4  emissiveColour  = Maths::Vector4(0.0f,0.0f,0.0f,1.0f);
		float usingAlbedoMap = 1.0f;
		float usingMetallicMap = 1.0f;
		float usingRoughnessMap = 1.0f;
		float usingNormalMap = 1.0f;
		float usingAOMap = 1.0f;
		float usingEmissiveMap = 1.0f;
		float workflow = PBR_WORKFLOW_SEPARATE_TEXTURES;
        float padding;
	};

	struct PBRMataterialTextures
	{
		Ref<Graphics::Texture2D> albedo;
		Ref<Graphics::Texture2D> normal;
		Ref<Graphics::Texture2D> metallic;
		Ref<Graphics::Texture2D> roughness;
		Ref<Graphics::Texture2D> ao;
		Ref<Graphics::Texture2D> emissive;
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
		Material(Ref<Graphics::Shader>& shader, const MaterialProperties& properties = MaterialProperties(), const PBRMataterialTextures& textures = PBRMataterialTextures());
		Material();
        
		NONCOPYABLE(Material);

		~Material();

		void SetRenderFlags(int flags) { m_RenderFlags = flags; }
		void SetRenderFlag(Material::RenderFlags flag) { m_RenderFlags |= static_cast<int>(flag); }
		void LoadPBRMaterial(const String& name, const String& path, const String& extension = ".png"); //TODO : Texture Parameters
		void LoadMaterial(const String& name, const String& path);
		void CreateDescriptorSet(Graphics::Pipeline* pipeline, int layoutID, bool pbr = true);

		void SetTextures(const PBRMataterialTextures& textures);
        void SetMaterialProperites(const MaterialProperties& properties);
        void UpdateMaterialPropertiesData();

		const PBRMataterialTextures& 	GetTextures() 		const { return m_PBRMaterialTextures; }
		Graphics::Shader* 				GetShader()			const { return m_Shader.get(); }
		Graphics::DescriptorSet* 		GetDescriptorSet() 	const { return m_DescriptorSet; }
		Graphics::Pipeline* 			GetPipeline()		const { return m_Pipeline; }
		int								GetRenderFlags()	const { return m_RenderFlags; }
		const String&					GetName()			const { return m_Name; }
		MaterialProperties*				GetProperties()		const { return m_MaterialProperties; }

		void OnImGui();
        
        static void InitDefaultTexture();
        static void ReleaseDefaultTexture();

	private:
		PBRMataterialTextures   			m_PBRMaterialTextures;
		Ref<Graphics::Shader>				m_Shader;
		Graphics::Pipeline* 				m_Pipeline;
		Graphics::DescriptorSet* 			m_DescriptorSet;
		Graphics::UniformBuffer* 			m_MaterialPropertiesBuffer;
		MaterialProperties*					m_MaterialProperties;
		u32									m_MaterialBufferSize;
        u8*									m_MaterialBufferData;
		String								m_Name;
        
        static Ref<Graphics::Texture2D> s_DefaultTexture;
	};
}
