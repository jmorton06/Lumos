#pragma once

#include "EditorPanel.h"
#include "Core/OS/Memory.h"

namespace Lumos
{
	class Entity;
    class HierarchyPanel : public EditorPanel
    {
    public:
        HierarchyPanel();
        ~HierarchyPanel();

        void DrawNode(Entity node);
        void OnImGui() override;
        bool IsParentOfEntity(Entity entity, Entity child);

    private:
        bool m_SelectUp;
        bool m_SelectDown;

        Arena* m_StringArena;
    };
}
