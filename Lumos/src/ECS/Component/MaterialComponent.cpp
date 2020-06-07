#include "lmpch.h"
#include "MaterialComponent.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/API/Texture.h"

#include <imgui/imgui.h>

namespace Lumos
{
    MaterialComponent::MaterialComponent()
    {
        m_Material = CreateRef<Material>();
    }
    
    MaterialComponent::MaterialComponent(Ref<Material>& material)
        : m_Material(material)
    {
    }
    
    MaterialComponent::~MaterialComponent()
    {
    }

    void MaterialComponent::SetAlbedoTexture(const String& path)
    {
        auto tex = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            auto& textures = m_Material->GetTextures();
            textures.albedo = tex;
            m_TexturesUpdated = true;
        }
    }

    void MaterialComponent::SetNormalTexture(const String& path)
    {
        auto tex = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            auto& textures = m_Material->GetTextures();
            textures.normal = tex;
            m_TexturesUpdated = true;
        }
    }

    void MaterialComponent::SetRoughnessTexture(const String& path)
    {
        auto tex = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            auto& textures = m_Material->GetTextures();
            textures.roughness = tex;
            m_TexturesUpdated = true;
        }
    }

    void MaterialComponent::SetMetallicTexture(const String& path)
    {
        auto tex = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            auto& textures = m_Material->GetTextures();
            textures.metallic = tex;
            m_TexturesUpdated = true;
        }
    }

    void MaterialComponent::SetAOTexture(const String& path)
    {
        auto tex = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            auto& textures = m_Material->GetTextures();
            textures.ao = tex;
            m_TexturesUpdated = true;
        }
    }

    void MaterialComponent::SetEmissiveTexture(const String& path)
    {
        auto tex = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
        if(tex)
        {
            auto& textures = m_Material->GetTextures();
            textures.emissive = tex;
            m_TexturesUpdated = true;
        }
    }
}
