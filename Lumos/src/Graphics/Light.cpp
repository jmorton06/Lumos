#include "lmpch.h"
#include "Light.h"

namespace Lumos
{
	namespace Graphics
	{
		Light::Light(const Maths::Vector3& direction, const Maths::Vector4& colour, float intensity, const LightType& type, const Maths::Vector3& position, float radius)
			: m_Direction(direction), m_Colour(colour), m_Position(position), m_Intensity(intensity), m_Radius(radius), m_Type(float(type))
		{
		}
	}
}

