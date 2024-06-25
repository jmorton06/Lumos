#include "Precompiled.h"
#include "IMGUIRenderer.h"
#include "GraphicsContext.h"
#include "Graphics/RHI/Texture.h"

#ifdef LUMOS_IMGUI
#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLIMGUIRenderer.h"
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKIMGUIRenderer.h"
#endif
#endif

namespace Lumos
{
    namespace Graphics
    {
        IMGUIRenderer* (*IMGUIRenderer::CreateFunc)(uint32_t, uint32_t, bool) = nullptr;

        IMGUIRenderer* IMGUIRenderer::Create(uint32_t width, uint32_t height, bool clearScreen)
        {
            LUMOS_ASSERT(CreateFunc, "No IMGUIRenderer Create Function");

            return CreateFunc(width, height, clearScreen);
        }

        ImGuiTextureID* IMGUIRenderer::AddTexture(Texture* texture, TextureType type, uint32_t level, uint32_t mip)
        {
            if(Graphics::GraphicsContext::GetRenderAPI() == RenderAPI::OPENGL)
                return (ImGuiTextureID*)texture->GetHandle();

            if(m_CurrentTextureIDIndex >= MAX_IMGUI_TEXTURES)
            {
                LUMOS_LOG_ERROR("Exceeded max imgui textures");
                return &m_TextureIDs[0];
            }

            m_TextureIDs[m_CurrentTextureIDIndex].texture = texture;
            m_TextureIDs[m_CurrentTextureIDIndex].type    = type;
            m_TextureIDs[m_CurrentTextureIDIndex].level   = level;
            m_TextureIDs[m_CurrentTextureIDIndex].mip     = mip;

            return &m_TextureIDs[m_CurrentTextureIDIndex++];
        }
    }
}
