#pragma once
#include "lmpch.h"
#include "Maths/Transform.h"

#include <jsonhpp/json.hpp>

namespace Lumos
{
	class LUMOS_EXPORT TransformComponent
	{
	public:
		TransformComponent(const Maths::Matrix4& matrix = Maths::Matrix4());

        void SetWorldMatrix(const Maths::Matrix4& matrix);
        void OnImGui();
		
		Ref<Maths::Transform>& GetTransform() { return m_Transform; }

		nlohmann::json Serialise();
		void Deserialise(nlohmann::json& data);

    private:
		Ref<Maths::Transform> m_Transform;
	};
}
