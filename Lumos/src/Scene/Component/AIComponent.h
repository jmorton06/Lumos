#pragma once
#include "lmpch.h"
#include "AI/AINode.h"

namespace Lumos
{
	class LUMOS_EXPORT AIComponent
	{
	public:
        AIComponent();
		explicit AIComponent(Ref<AINode>& aiNode);

		void OnImGui();
    private:
        Ref<AINode> m_AINode;
	};
}
