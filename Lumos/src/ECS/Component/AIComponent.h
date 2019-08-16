#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "AI/AINode.h"

namespace Lumos
{
	class LUMOS_EXPORT AIComponent : public LumosComponent
	{
	public:
        AIComponent();
		explicit AIComponent(Ref<AINode>& aiNode);

		void OnIMGUI() override;
		nlohmann::json Serialise() override { return nullptr; };
		void Deserialise(nlohmann::json& data) override {};
    private:
        Ref<AINode> m_AINode;
	};
}
