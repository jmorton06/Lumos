#pragma once

#include "AI/AINode.h"

namespace Lumos
{
    class LUMOS_EXPORT AIComponent
    {
    public:
        AIComponent();
        explicit AIComponent(SharedRef<AINode>& aiNode);

        void OnImGui();

    private:
        SharedRef<AINode> m_AINode;
    };
}
