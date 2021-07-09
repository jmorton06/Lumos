#include "Precompiled.h"
#include "AIComponent.h"

#include <imgui/imgui.h>

namespace Lumos
{
    AIComponent::AIComponent()
    {
        m_AINode = CreateSharedRef<AINode>();
    }

    AIComponent::AIComponent(SharedRef<AINode>& aiNode)
        : m_AINode(aiNode)
    {
    }

    void AIComponent::OnImGui()
    {
    }

}
