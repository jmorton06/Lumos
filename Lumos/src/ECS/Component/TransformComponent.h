#pragma once
#include "lmpch.h"
#include "LumosComponent.h"
#include "Maths/Transform.h"

namespace Lumos
{
	class LUMOS_EXPORT TransformComponent : public LumosComponent
	{
	public:
		TransformComponent(const Maths::Matrix4& matrix = Maths::Matrix4());

        void SetWorldMatrix(const Maths::Matrix4& matrix);
        void OnImGui() override;
		
		Ref<Maths::Transform>& GetTransform() { return m_Transform; }

		nlohmann::json Serialise() override;
		void Deserialise(nlohmann::json& data) override;

		SETUPCOMPOMENT(TransformComponent);

    private:
		Ref<Maths::Transform> m_Transform;
	};
}
