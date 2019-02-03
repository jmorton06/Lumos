#pragma once
#include "LM.h"

namespace Lumos
{
    class Texture;
    class TextureCube;
    class TextureDepthArray;
    class GBuffer;
    
    class LUMOS_EXPORT RenderManager
    {
    public:
        RenderManager() { Reset(); };
        ~RenderManager() {};
        
        void Reset();
        void OnResize() {};
        
        bool GetReflectSkyBox() const { return m_ReflectSkyBox; };
        bool GetUseShadowMap() const { return m_UseShadowMap; };
        uint GetNumShadowMaps() const { return m_NumShadowMaps; };
        TextureCube* GetSkyBox() const { return m_SkyBoxTexture; };
        TextureDepthArray* GetShadowTexture() const { return m_ShadowTexture; };
        
        void SetReflectSkyBox(bool reflect) { m_ReflectSkyBox = reflect; }
        void SetUseShadowMap(bool shadow) { m_UseShadowMap = shadow; }
        void SetNumShadowMaps(uint num) { m_NumShadowMaps = num; }
        void SetSkyBoxTexture(TextureCube* cube) { m_SkyBoxTexture = cube; }
        void SetTextureDepthArray(TextureDepthArray* texture) { m_ShadowTexture = texture; }
        
    private:
        bool m_ReflectSkyBox = false;
        bool m_UseShadowMap = false;
        uint m_NumShadowMaps = 4;
        TextureCube* m_SkyBoxTexture = nullptr;
        TextureDepthArray* m_ShadowTexture = nullptr;
        Texture* m_ScreenTexture = nullptr;
        
        GBuffer* m_GBuffer = nullptr;
    };
}

