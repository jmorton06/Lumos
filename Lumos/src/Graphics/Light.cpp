#include "LM.h"
#include "Light.h"

namespace Lumos
{
	namespace graphics
	{
		Light::Light(const maths::Vector3& direction, const maths::Vector4& colour, float intensity, const LightType& type, const maths::Vector3& position, float radius)
			: m_Direction(direction), m_Intensity(intensity), m_Colour(colour), p0(0.0f), p1(0.0f),p2(0.0f), m_Position(position), m_Radius(radius), m_LightType(static_cast<float>(type))
		{
		}
	}
}

