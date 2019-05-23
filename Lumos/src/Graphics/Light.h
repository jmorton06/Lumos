#pragma once
#include "LM.h"
#include "Maths/Maths.h"

namespace lumos
{
	namespace graphics
	{
		enum class LUMOS_EXPORT LightType
		{
			DirectionalLight = 0,
			SpotLight = 1,
			PointLight = 2
		};

		struct LUMOS_EXPORT MEM_ALIGN Light
		{
			Light(const maths::Vector3& direction, const maths::Vector4& colour = maths::Vector4(1.0f), float intensity = 1.0f, const LightType& type = LightType::DirectionalLight, const maths::Vector3& position = maths::Vector3(), float radius = 1.0f);

			maths::Vector4   m_Colour;
			maths::Vector4   m_Position;
			maths::Vector4   m_Direction;
			float m_Intensity;
			float m_Radius;
			float m_Type;
			float p0 = 0.0f;
		};
	}
}
