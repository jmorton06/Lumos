#pragma once
#include "lmpch.h"
#include "Graphics/Material.h"

namespace Lumos
{
    class LUMOS_EXPORT MaterialComponent
    {
    public:
        MaterialComponent();
        explicit MaterialComponent(Ref<Material>& material);
        virtual ~MaterialComponent();

        void SetAlbedoTexture(const String& path);
        void SetNormalTexture(const String& path);
        void SetRoughnessTexture(const String& path);
        void SetMetallicTexture(const String& path);
        void SetAOTexture(const String& path);
        void SetEmissiveTexture(const String& path);
        
        const Ref<Material>& GetMaterial() const { return m_Material; }
		bool& GetActive() { return m_Active; }
		const bool& GetActive() const { return m_Active; }
    
        bool& GetTexturesUpdated() { return m_TexturesUpdated; }
        const bool& GetTexturesUpdated() const { return m_TexturesUpdated; }
        void SetTexturesUpdated(bool updated) { m_TexturesUpdated = updated; }

    private:
        Ref<Material> m_Material;
		bool m_Active = true;
        bool m_TexturesUpdated = false;
    };
}
