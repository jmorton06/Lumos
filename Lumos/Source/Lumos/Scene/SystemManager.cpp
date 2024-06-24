#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "SystemManager.h"
#include <imgui/imgui.h>

namespace Lumos
{
    SystemManager::SystemManager()
    {
        m_Arena         = ArenaAlloc(Kilobytes(64));
        m_Systems       = {};
        m_Systems.arena = m_Arena;
    }
    SystemManager::~SystemManager()
    {
        ForHashMapEach(size_t, ISystem*, &m_Systems, it)
        {
            ISystem* value = *it.value;
            delete value;
        }
    }

    void SystemManager::OnImGui()
    {
        ForHashMapEach(size_t, ISystem*, &m_Systems, it)
        {
            ISystem* value = *it.value;
            if(ImGui::TreeNode(value->GetName().c_str()))
            {
                value->OnImGui();
                ImGui::TreePop();
            }
        }
    }
}
