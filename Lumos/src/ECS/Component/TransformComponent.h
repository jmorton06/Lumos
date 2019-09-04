#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "Maths/Transform.h"

namespace Lumos
{
	class LUMOS_EXPORT TransformComponent : public LumosComponent
	{
	public:
		explicit TransformComponent();
		explicit TransformComponent(const Maths::Matrix4& matrix);

        void SetWorldMatrix(const Maths::Matrix4& matrix);
        void OnIMGUI() override;
		
		Maths::Transform& GetTransform() { return m_Transform; }

		nlohmann::json Serialise() override;
		void Deserialise(nlohmann::json& data) override;

    private:
		Maths::Transform m_Transform;
	};
}
