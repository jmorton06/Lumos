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
        explicit MaterialComponent(std::shared_ptr<Material>& material);
        virtual ~MaterialComponent();
        
        static ComponentType GetStaticType()
        {
            static ComponentType type(ComponentType::Material);
            return type;
        }
        
        void OnUpdateComponent(float dt) override;
        
        inline virtual ComponentType GetType() const override { return GetStaticType(); }
        
        void OnIMGUI() override;
        
        const std::shared_ptr<Material>& GetMaterial() const { return m_Material; }
    private:
        std::shared_ptr<Material> m_Material;
        ImGui::FileBrowser* m_FileDialog;
    };
}
