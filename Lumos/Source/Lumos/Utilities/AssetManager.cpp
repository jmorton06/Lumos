#include "Precompiled.h"
#include "AssetManager.h"
#include "Core/Application.h"

namespace Lumos
{
    inline static std::vector<std::future<void>> m_Futures;

    static void LoadTexture2D(Graphics::Texture2D* tex, const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        uint32_t width, height, channels;
        bool hdr;
        uint32_t bits;
        uint8_t* data = Lumos::LoadImageFromFile(path, &width, &height, &bits, &hdr);

        Graphics::TextureDesc desc;
        desc.format = bits / 4 == 8 ? Graphics::RHIFormat::R8G8B8A8_Unorm : Graphics::RHIFormat::R32G32B32A32_Float;

        Application::Get().SubmitToMainThread([tex, path, width, height, data, desc]()
                                              { tex->Load(width, height, data, desc); });
    }

    bool TextureLibrary::Load(const std::string& filePath, SharedPtr<Graphics::Texture2D>& texture)
    {
        texture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::Create({}, 1, 1));
        m_Futures.push_back(std::async(std::launch::async, &LoadTexture2D, texture.get(), filePath));
        return true;
    }

    void TextureLibrary::Destroy()
    {
        m_Futures.clear();
        typename MapType::iterator itr = m_NameResourceMap.begin();

        if(m_ReleaseFunc)
        {
            while(itr != m_NameResourceMap.end())
            {
                m_ReleaseFunc((itr->second.data));
                ++itr;
            }
        }
        m_NameResourceMap.clear();
    }
}
