#pragma once
#include "LM.h"
#include "LumosComponent.h"

namespace Lumos
{
	class AINode;

	class LUMOS_EXPORT AIComponent : public LumosComponent
	{
	public:
		std::shared_ptr<AINode> m_AINode;
	public:
		explicit AIComponent(std::shared_ptr<AINode>& aiNode);

		void OnIMGUI() override;
	};
}
