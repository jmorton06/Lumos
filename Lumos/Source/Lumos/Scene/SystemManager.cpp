#include "Precompiled.h"
#include "SystemManager.h"
#include <imgui/imgui.h>

namespace Lumos
{
    SystemManager::SystemManager()
    {
        m_Arena         = ArenaAlloc(Kilobytes(64));
        m_Systems       = {};
        m_Systems.arena = m_Arena;

        m_Mutex = PushArray(m_Arena, Mutex, 1);
        MutexInit(m_Mutex);
    }
    SystemManager::~SystemManager()
    {
        ForHashMapEach(size_t, ISystem*, &m_Systems, it)
        {
            ISystem* value = *it.value;
            delete value;
        }

        MutexDestroy(m_Mutex);
    }

    void SystemManager::OnImGui()
    {
        ForHashMapEach(size_t, ISystem*, &m_Systems, it)
        {
            ISystem* value = *it.value;
            if(ImGui::TreeNode(value->GetName()))
            {
                value->OnImGui();
                ImGui::TreePop();
            }
        }
    }
}
