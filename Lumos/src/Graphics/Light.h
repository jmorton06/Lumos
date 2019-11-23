#pragma once
#include "lmpch.h"
#include "Maths/Maths.h"

namespace Lumos
{
	namespace Graphics
	{
		enum class LUMOS_EXPORT LightType
		{
			DirectionalLight = 0,
			SpotLight = 1,
			PointLight = 2
		};

		struct LUMOS_EXPORT MEM_ALIGN Light
		{
            Light(const Maths::Vector3& direction = Maths::Vector3(0.0f), const Maths::Vector4& colour = Maths::Vector4(1.0f), float intensity = 1.0f, const LightType& type = LightType::DirectionalLight, const Maths::Vector3& position = Maths::Vector3(), float radius = 1.0f, float angle = 0.0f);

			void OnImGui();

			Maths::Vector4   m_Colour;
			Maths::Vector4   m_Position;
			Maths::Vector4   m_Direction;
			float m_Intensity;
			float m_Radius;
			float m_Type;
			float m_Angle;
		};
	}
}
