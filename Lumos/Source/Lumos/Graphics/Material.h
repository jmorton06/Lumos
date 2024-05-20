#pragma once
#include "RHI/Texture.h"
#include "RHI/Shader.h"

namespace Lumos
{
    namespace Graphics
    {
        class DescriptorSet;

        const float PBR_WORKFLOW_SEPARATE_TEXTURES  = 0.0f;
        const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 1.0f;
        const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 2.0f;

        struct MaterialProperties
        {
            glm::vec4 albedoColour   = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            float roughness          = 0.7f;
            float metallic           = 0.7f;
            float reflectance        = 0.3f;
            float emissive           = 0.0f;
            float albedoMapFactor    = 1.0f;
            float metallicMapFactor  = 1.0f;
            float roughnessMapFactor = 1.0f;
            float normalMapFactor    = 1.0f;
            float emissiveMapFactor  = 1.0f;
            float occlusionMapFactor = 1.0f;
            float alphaCutoff        = 0.4f;
            float workflow           = PBR_WORKFLOW_SEPARATE_TEXTURES;
            float padding            = 0.0f;
        };

        struct PBRMataterialTextures
        {
            SharedPtr<Texture2D> albedo;
            SharedPtr<Texture2D> normal;
            SharedPtr<Texture2D> metallic;
            SharedPtr<Texture2D> roughness;
            SharedPtr<Texture2D> ao;
            SharedPtr<Texture2D> emissive;
        };

        class LUMOS_EXPORT Material
        {
            template <typename Archive>
            friend void save(Archive& archive, const Material& material);

            template <typename Archive>
            friend void load(Archive& archive, Material& material);

        public:
            enum class RenderFlags
            {
                NONE           = 0,
                DEPTHTEST      = BIT(0),
                WIREFRAME      = BIT(1),
                FORWARDRENDER  = BIT(2),
                DEFERREDRENDER = BIT(3),
                NOSHADOW       = BIT(4),
                TWOSIDED       = BIT(5),
                ALPHABLEND     = BIT(6)
            };

        public:
            Material(SharedPtr<Shader>& shader, const MaterialProperties& properties = MaterialProperties(), const PBRMataterialTextures& textures = PBRMataterialTextures());
            Material();

            ~Material();

            void LoadPBRMaterial(const std::string& name, const std::string& path, const std::string& extension = ".png"); // TODO : Texture Parameters
            void LoadMaterial(const std::string& name, const std::string& path);
            void CreateDescriptorSet(int layoutID, bool pbr = true);

            void SetTextures(const PBRMataterialTextures& textures);
            void SetMaterialProperites(const MaterialProperties& properties);
            void UpdateMaterialPropertiesData();
            void UpdateDescriptorSet();

            void SetName(const std::string& name) { m_Name = name; }

            void SetAlbedoTexture(const std::string& path);
            void SetNormalTexture(const std::string& path);
            void SetRoughnessTexture(const std::string& path);
            void SetMetallicTexture(const std::string& path);
            void SetAOTexture(const std::string& path);
            void SetEmissiveTexture(const std::string& path);
            void SetShader(SharedPtr<Shader>& shader)
            {
                m_Shader          = shader;
                m_TexturesUpdated = true; // TODO
            }

            bool& GetTexturesUpdated() { return m_TexturesUpdated; }
            const bool& GetTexturesUpdated() const { return m_TexturesUpdated; }
            void SetTexturesUpdated(bool updated) { m_TexturesUpdated = updated; }
            PBRMataterialTextures& GetTextures() { return m_PBRMaterialTextures; }
            const PBRMataterialTextures& GetTextures() const { return m_PBRMaterialTextures; }
            SharedPtr<Shader> GetShader() const { return m_Shader; }
            DescriptorSet* GetDescriptorSet() const { return m_DescriptorSet; }
            const std::string& GetName() const { return m_Name; }
            MaterialProperties* GetProperties() const { return m_MaterialProperties; }

            void Bind();
            void SetShader(const std::string& filePath);

            static void InitDefaultTexture();
            static void ReleaseDefaultTexture();

            uint32_t GetFlags() const { return m_Flags; };
            bool GetFlag(RenderFlags flag) const { return (uint32_t)flag & m_Flags; };
            void SetFlag(RenderFlags flag, bool value = true)
            {
                if(value)
                {
                    m_Flags |= (uint32_t)flag;
                }
                else
                {
                    m_Flags &= ~(uint32_t)flag;
                }
            };

            static SharedPtr<Texture2D> GetDefaultTexture() { return s_DefaultTexture; }
            const std::string& GetMaterialPath() const { return m_MaterialPath; }
            void SetMaterialPath(const std::string& path) { m_MaterialPath = path; }

        private:
            PBRMataterialTextures m_PBRMaterialTextures;
            SharedPtr<Shader> m_Shader;
            DescriptorSet* m_DescriptorSet;
            MaterialProperties* m_MaterialProperties;
            uint32_t m_MaterialBufferSize;
            std::string m_Name;
            bool m_TexturesUpdated = false;
            uint32_t m_Flags;

            std::string m_MaterialPath;

            static SharedPtr<Texture2D> s_DefaultTexture;
        };
    }
}
