#pragma once
#include "LM.h"
#include "LumosComponent.h"

namespace Lumos
{
	class AINode;

	class LUMOS_EXPORT AIComponent : public LumosComponent
	{
	public:
        AIComponent();
		explicit AIComponent(std::shared_ptr<AINode>& aiNode);

		void OnIMGUI() override;
		nlohmann::json Serialise() override { return nullptr; };
		void Deserialise(nlohmann::json& data) override {};
    private:
        std::shared_ptr<AINode> m_AINode;
	};
}
