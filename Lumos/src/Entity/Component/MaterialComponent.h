#pragma once
#include "LM.h"
#include "LumosComponent.h"

namespace ImGui
{
    class FileBrowser;
}

namespace Lumos
{
    class Material;
    
    class LUMOS_EXPORT MaterialComponent : public LumosComponent
    {
    public:
        MaterialComponent();
        explicit MaterialComponent(std::shared_ptr<Material>& material);
        virtual ~MaterialComponent();
        
        void OnUpdateComponent(float dt) override;
        
        void OnIMGUI() override;
        
        const std::shared_ptr<Material>& GetMaterial() const { return m_Material; }
    private:
        std::shared_ptr<Material> m_Material;
        ImGui::FileBrowser* m_FileDialog;
    };
}
