#include "Precompiled.h"
#include "Material.h"
#include "Graphics/RHI/Shader.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/DescriptorSet.h"
#include "Graphics/RHI/Pipeline.h"
#include "Graphics/RHI/UniformBuffer.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Core/OS/FileSystem.h"
#include "Core/VFS.h"
#include "Core/Application.h"
#include "Renderers/ForwardRenderer.h"

#include <imgui/imgui.h>

namespace Lumos::Graphics
{

    SharedPtr<Graphics::Texture2D> Material::s_DefaultTexture = nullptr;

    Material::Material(SharedPtr<Graphics::Shader>& shader, const MaterialProperties& properties, const PBRMataterialTextures& textures)
        : m_PBRMaterialTextures(textures)
        , m_Shader(shader)
    {
        LUMOS_PROFILE_FUNCTION();

        m_Flags = 0;
        SetFlag(RenderFlags::DEPTHTEST);
        m_DescriptorSet = nullptr;
        m_MaterialProperties = new MaterialProperties();
        m_MaterialBufferSize = sizeof(MaterialProperties);

        m_DescriptorSet = nullptr;

        SetMaterialProperites(properties);
    }

    Material::Material()
        : m_Shader(nullptr)
    {
        LUMOS_PROFILE_FUNCTION();

        m_Flags = 0;
        SetFlag(RenderFlags::DEPTHTEST);
        m_DescriptorSet = nullptr;
        m_MaterialProperties = new MaterialProperties();
        m_PBRMaterialTextures.albedo = nullptr;
        m_MaterialBufferSize = sizeof(MaterialProperties);
    }

    Material::~Material()
    {
        LUMOS_PROFILE_FUNCTION();
        delete m_DescriptorSet;
        delete m_MaterialProperties;
    }

    void Material::SetTextures(const PBRMataterialTextures& textures)
    {
        LUMOS_PROFILE_FUNCTION();
        m_PBRMaterialTextures.albedo = textures.albedo;
        m_PBRMaterialTextures.normal = textures.normal;
        m_PBRMaterialTextures.roughness = textures.roughness;
        m_PBRMaterialTextures.metallic = textures.metallic;
        m_PBRMaterialTextures.ao = textures.ao;
        m_PBRMaterialTextures.emissive = textures.emissive;
    }

    bool FileExists(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();

        std::string physicalPath;

        VFS::Get().ResolvePhysicalPath(path, physicalPath);
        return FileSystem::FileExists(physicalPath);
    }

    void Material::LoadPBRMaterial(const std::string& name, const std::string& path, const std::string& extension)
    {
        LUMOS_PROFILE_FUNCTION();

        m_Name = name;
        m_PBRMaterialTextures = PBRMataterialTextures();
        auto params = Graphics::TextureParameters(Graphics::TextureFormat::RGBA8, Graphics::TextureFilter::LINEAR, Graphics::TextureFilter::LINEAR, Graphics::TextureWrap::CLAMP_TO_EDGE);

        auto filePath = path + "/" + name + "/albedo" + extension;

        if(FileExists(filePath))
            m_PBRMaterialTextures.albedo = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/albedo" + extension, params));

        filePath = path + "/" + name + "/normal" + extension;

        if(FileExists(filePath))
            m_PBRMaterialTextures.normal = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/normal" + extension, params));

        filePath = path + "/" + name + "/roughness" + extension;

        if(FileExists(filePath))
            m_PBRMaterialTextures.roughness = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/roughness" + extension, params));

        filePath = path + "/" + name + "/metallic" + extension;

        if(FileExists(filePath))
            m_PBRMaterialTextures.metallic = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/metallic" + extension, params));

        filePath = path + "/" + name + "/ao" + extension;

        if(FileExists(filePath))
            m_PBRMaterialTextures.ao = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/ao" + extension, params));

        filePath = path + "/" + name + "/emissive" + extension;

        if(FileExists(filePath))
            m_PBRMaterialTextures.emissive = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path + "/" + name + "/emissive" + extension, params));
    }

    void Material::LoadMaterial(const std::string& name, const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();

        m_Name = name;
        m_PBRMaterialTextures = PBRMataterialTextures();
        m_PBRMaterialTextures.albedo = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path));
        m_PBRMaterialTextures.normal = nullptr;
        m_PBRMaterialTextures.roughness = nullptr;
        m_PBRMaterialTextures.metallic = nullptr;
        m_PBRMaterialTextures.ao = nullptr;
        m_PBRMaterialTextures.emissive = nullptr;
    }

    void Material::UpdateMaterialPropertiesData()
    {
        if(!m_DescriptorSet)
            return;

        m_DescriptorSet->SetUniformBufferData("UniformMaterialData", *&m_MaterialProperties);
        m_DescriptorSet->Update();
    }

    void Material::SetMaterialProperites(const MaterialProperties& properties)
    {
        LUMOS_PROFILE_FUNCTION();

        m_MaterialProperties->albedoColour = properties.albedoColour;
        m_MaterialProperties->metallicColour = properties.metallicColour;
        m_MaterialProperties->roughnessColour = properties.roughnessColour;
        m_MaterialProperties->usingAlbedoMap = properties.usingAlbedoMap;
        m_MaterialProperties->usingNormalMap = properties.usingNormalMap;
        m_MaterialProperties->usingMetallicMap = properties.usingMetallicMap;
        m_MaterialProperties->usingRoughnessMap = properties.usingRoughnessMap;
        m_MaterialProperties->usingAOMap = properties.usingAOMap;
        m_MaterialProperties->usingEmissiveMap = properties.usingEmissiveMap;
        m_MaterialProperties->workflow = properties.workflow;
        m_MaterialProperties->emissiveColour = properties.emissiveColour;

        UpdateMaterialPropertiesData();
    }

    void Material::CreateDescriptorSet(int layoutID, bool pbr)
    {
        LUMOS_PROFILE_FUNCTION();

        if(m_DescriptorSet)
            delete m_DescriptorSet;

        if(!m_Shader)
        {
            //If no shader then set it to the default pbr shader
            //TODO default to forward
            m_Shader = Application::Get().GetShaderLibrary()->GetResource("ForwardPBR");
        }

        Graphics::DescriptorDesc descriptorDesc;
        descriptorDesc.layoutIndex = layoutID;
        descriptorDesc.shader = m_Shader.get();

        m_DescriptorSet = Graphics::DescriptorSet::Create(descriptorDesc);

        if(m_PBRMaterialTextures.albedo != nullptr)
        {
            m_DescriptorSet->SetTexture("u_AlbedoMap", m_PBRMaterialTextures.albedo.get());
        }
        else
        {
            m_DescriptorSet->SetTexture("u_AlbedoMap", s_DefaultTexture.get());
            m_MaterialProperties->usingAlbedoMap = 0.0f;
        }

        if(pbr)
        {
            m_DescriptorSet->SetTexture("u_MetallicMap", m_PBRMaterialTextures.metallic ? m_PBRMaterialTextures.metallic.get() : s_DefaultTexture.get());

            if(!m_PBRMaterialTextures.metallic)
                m_MaterialProperties->usingMetallicMap = 0.0f;

            m_DescriptorSet->SetTexture("u_RoughnessMap", m_PBRMaterialTextures.roughness ? m_PBRMaterialTextures.roughness.get() : s_DefaultTexture.get());

            if(!m_PBRMaterialTextures.roughness)
                m_MaterialProperties->usingRoughnessMap = 0.0f;

            if(m_PBRMaterialTextures.normal != nullptr)
            {
                m_DescriptorSet->SetTexture("u_NormalMap", m_PBRMaterialTextures.normal.get());
            }
            else
            {
                m_DescriptorSet->SetTexture("u_NormalMap", s_DefaultTexture.get());
                m_MaterialProperties->usingNormalMap = 0.0f;
            }

            if(m_PBRMaterialTextures.ao != nullptr)
            {
                m_DescriptorSet->SetTexture("u_AOMap", m_PBRMaterialTextures.ao.get());
            }
            else
            {
                m_DescriptorSet->SetTexture("u_AOMap", s_DefaultTexture.get());
                m_MaterialProperties->usingAOMap = 0.0f;
            }

            if(m_PBRMaterialTextures.emissive != nullptr)
            {
                m_DescriptorSet->SetTexture("u_EmissiveMap", m_PBRMaterialTextures.emissive.get());
            }
            else
            {
                m_DescriptorSet->SetTexture("u_EmissiveMap", s_DefaultTexture.get());
                m_MaterialProperties->usingEmissiveMap = 0.0f;
            }

            UpdateMaterialPropertiesData();
        }

        m_DescriptorSet->Update();
    }

    void Material::Bind()
    {
        LUMOS_PROFILE_FUNCTION();

        if(m_DescriptorSet == nullptr || GetTexturesUpdated())
        {
            CreateDescriptorSet(1);
            SetTexturesUpdated(false);
        }
    }

    void Material::SetShader(const std::string& filePath)
    {
        m_Shader = Application::Get().GetShaderLibrary()->GetResource(filePath);
    }

    void Material::InitDefaultTexture()
    {
        LUMOS_PROFILE_FUNCTION();

        uint32_t whiteTextureData = 0xffffffff;
        s_DefaultTexture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromSource(1, 1, &whiteTextureData));
    }

    void Material::ReleaseDefaultTexture()
    {
        LUMOS_PROFILE_FUNCTION();

        s_DefaultTexture.reset();
    }

    void Material::SetAlbedoTexture(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();

        auto tex = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            m_PBRMaterialTextures.albedo = tex;
            m_TexturesUpdated = true;
        }
    }

    void Material::SetNormalTexture(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();

        auto tex = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            m_PBRMaterialTextures.normal = tex;
            m_TexturesUpdated = true;
        }
    }

    void Material::SetRoughnessTexture(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();

        auto tex = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            m_PBRMaterialTextures.roughness = tex;
            m_TexturesUpdated = true;
        }
    }

    void Material::SetMetallicTexture(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();

        auto tex = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            m_PBRMaterialTextures.metallic = tex;
            m_TexturesUpdated = true;
        }
    }

    void Material::SetAOTexture(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();

        auto tex = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            m_PBRMaterialTextures.ao = tex;
            m_TexturesUpdated = true;
        }
    }

    void Material::SetEmissiveTexture(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();

        auto tex = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            m_PBRMaterialTextures.emissive = tex;
            m_TexturesUpdated = true;
        }
    }
}
