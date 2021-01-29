#pragma once
#include "Maths/Maths.h"
#include "API/Texture.h"
#include "API/Shader.h"
#include <cereal/cereal.hpp>

namespace Lumos
{
	namespace Graphics
	{
		class Pipeline;
		class DescriptorSet;
		class UniformBuffer;

		const float PBR_WORKFLOW_SEPARATE_TEXTURES = 0.0f;
		const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 1.0f;
		const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 2.0f;

		struct MaterialProperties
		{
			Maths::Vector4 albedoColour = Maths::Vector4(1.0f, 0.0f, 1.0f, 1.0f);
			Maths::Vector4 roughnessColour = Maths::Vector4(1.0f, 0.0f, 1.0f, 1.0f);
			Maths::Vector4 metallicColour = Maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f);
			Maths::Vector4 emissiveColour = Maths::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
			float usingAlbedoMap = 1.0f;
			float usingMetallicMap = 1.0f;
			float usingRoughnessMap = 1.0f;
			float usingNormalMap = 1.0f;
			float usingAOMap = 1.0f;
			float usingEmissiveMap = 1.0f;
			float workflow = PBR_WORKFLOW_SEPARATE_TEXTURES;
			float padding = 0.0f;
		};

		struct PBRMataterialTextures
		{
			Ref<Texture2D> albedo;
			Ref<Texture2D> normal;
			Ref<Texture2D> metallic;
			Ref<Texture2D> roughness;
			Ref<Texture2D> ao;
			Ref<Texture2D> emissive;
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
			Material(Ref<Shader>& shader, const MaterialProperties& properties = MaterialProperties(), const PBRMataterialTextures& textures = PBRMataterialTextures());
			Material();

			~Material();

			void SetRenderFlags(int flags)
			{
				m_RenderFlags = flags;
			}
			void SetRenderFlag(Material::RenderFlags flag)
			{
				m_RenderFlags |= static_cast<int>(flag);
			}
			void LoadPBRMaterial(const std::string& name, const std::string& path, const std::string& extension = ".png"); //TODO : Texture Parameters
			void LoadMaterial(const std::string& name, const std::string& path);
			void CreateDescriptorSet(Pipeline* pipeline, int layoutID, bool pbr = true);

			void SetTextures(const PBRMataterialTextures& textures);
			void SetMaterialProperites(const MaterialProperties& properties);
			void UpdateMaterialPropertiesData();
            
            void SetAlbedoTexture(const std::string& path);
            void SetNormalTexture(const std::string& path);
            void SetRoughnessTexture(const std::string& path);
            void SetMetallicTexture(const std::string& path);
            void SetAOTexture(const std::string& path);
            void SetEmissiveTexture(const std::string& path);
            
            bool& GetTexturesUpdated()
            {
                return m_TexturesUpdated;
            }
            
            const bool& GetTexturesUpdated() const
            {
                return m_TexturesUpdated;
            }
            
            void SetTexturesUpdated(bool updated)
            {
                m_TexturesUpdated = updated;
            }

			PBRMataterialTextures& GetTextures()
			{
				return m_PBRMaterialTextures;
			}
			const PBRMataterialTextures& GetTextures() const
			{
				return m_PBRMaterialTextures;
			}
			Shader* GetShader() const
			{
				return m_Shader.get();
			}
			DescriptorSet* GetDescriptorSet() const
			{
				return m_DescriptorSet;
			}
			Pipeline* GetPipeline() const
			{
				return m_Pipeline;
			}
			int GetRenderFlags() const
			{
				return m_RenderFlags;
			}
			const std::string& GetName() const
			{
				return m_Name;
			}
			MaterialProperties* GetProperties() const
			{
				return m_MaterialProperties;
			}

			static void InitDefaultTexture();
			static void ReleaseDefaultTexture();

			template<typename Archive>
			void save(Archive& archive) const
			{
				archive(cereal::make_nvp("Albedo", m_PBRMaterialTextures.albedo ? m_PBRMaterialTextures.albedo->GetFilepath() : ""),
					cereal::make_nvp("Normal", m_PBRMaterialTextures.normal ? m_PBRMaterialTextures.normal->GetFilepath() : ""),
					cereal::make_nvp("Metallic", m_PBRMaterialTextures.metallic ? m_PBRMaterialTextures.metallic->GetFilepath() : ""),
					cereal::make_nvp("Roughness", m_PBRMaterialTextures.roughness ? m_PBRMaterialTextures.roughness->GetFilepath() : ""),
					cereal::make_nvp("Ao", m_PBRMaterialTextures.ao ? m_PBRMaterialTextures.ao->GetFilepath() : ""),
					cereal::make_nvp("Emissive", m_PBRMaterialTextures.emissive ? m_PBRMaterialTextures.emissive->GetFilepath() : ""),
					cereal::make_nvp("albedoColour", m_MaterialProperties->albedoColour),
					cereal::make_nvp("roughnessColour", m_MaterialProperties->roughnessColour),
					cereal::make_nvp("metallicColour", m_MaterialProperties->metallicColour),
					cereal::make_nvp("emissiveColour", m_MaterialProperties->emissiveColour),
					cereal::make_nvp("usingAlbedoMap", m_MaterialProperties->usingAlbedoMap),
					cereal::make_nvp("usingMetallicMap", m_MaterialProperties->usingMetallicMap),
					cereal::make_nvp("usingRoughnessMap", m_MaterialProperties->usingRoughnessMap),
					cereal::make_nvp("usingNormalMap", m_MaterialProperties->usingNormalMap),
					cereal::make_nvp("usingAOMap", m_MaterialProperties->usingAOMap),
					cereal::make_nvp("usingEmissiveMap", m_MaterialProperties->usingEmissiveMap),
					cereal::make_nvp("workflow", m_MaterialProperties->workflow));
			}

			template<typename Archive>
			void load(Archive& archive)
			{
				std::string albedoFilePath;
				std::string normalFilePath;
				std::string roughnessFilePath;
				std::string metallicFilePath;
				std::string emissiveFilePath;
				std::string aoFilePath;

				archive(cereal::make_nvp("Albedo", albedoFilePath),
					cereal::make_nvp("Normal", normalFilePath),
					cereal::make_nvp("Metallic", metallicFilePath),
					cereal::make_nvp("Roughness", roughnessFilePath),
					cereal::make_nvp("Ao", aoFilePath),
					cereal::make_nvp("Emissive", emissiveFilePath),
					cereal::make_nvp("albedoColour", m_MaterialProperties->albedoColour),
					cereal::make_nvp("roughnessColour", m_MaterialProperties->roughnessColour),
					cereal::make_nvp("metallicColour", m_MaterialProperties->metallicColour),
					cereal::make_nvp("emissiveColour", m_MaterialProperties->emissiveColour),
					cereal::make_nvp("usingAlbedoMap", m_MaterialProperties->usingAlbedoMap),
					cereal::make_nvp("usingMetallicMap", m_MaterialProperties->usingMetallicMap),
					cereal::make_nvp("usingRoughnessMap", m_MaterialProperties->usingRoughnessMap),
					cereal::make_nvp("usingNormalMap", m_MaterialProperties->usingNormalMap),
					cereal::make_nvp("usingAOMap", m_MaterialProperties->usingAOMap),
					cereal::make_nvp("usingEmissiveMap", m_MaterialProperties->usingEmissiveMap),
					cereal::make_nvp("workflow", m_MaterialProperties->workflow));

				if(!albedoFilePath.empty())
					m_PBRMaterialTextures.albedo = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("albedo", albedoFilePath));
				if(!normalFilePath.empty())
					m_PBRMaterialTextures.normal = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("roughness", normalFilePath));
				if(!metallicFilePath.empty())
					m_PBRMaterialTextures.metallic = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("metallic", metallicFilePath));
				if(!roughnessFilePath.empty())
					m_PBRMaterialTextures.roughness = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("roughness", roughnessFilePath));
				if(!emissiveFilePath.empty())
					m_PBRMaterialTextures.emissive = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("emissive", emissiveFilePath));
				if(!aoFilePath.empty())
					m_PBRMaterialTextures.ao = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("ao", aoFilePath));
			}

		private:
			PBRMataterialTextures m_PBRMaterialTextures;
			Ref<Shader> m_Shader;
			Pipeline* m_Pipeline;
			DescriptorSet* m_DescriptorSet;
			UniformBuffer* m_MaterialPropertiesBuffer;
			MaterialProperties* m_MaterialProperties;
			uint32_t m_MaterialBufferSize;
			uint8_t* m_MaterialBufferData;
			std::string m_Name;
            bool m_TexturesUpdated = false;

			static Ref<Texture2D> s_DefaultTexture;
		};
	}
}
