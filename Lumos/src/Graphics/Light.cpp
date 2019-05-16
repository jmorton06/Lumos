#include "LM.h"
#include "Light.h"

namespace Lumos
{
	namespace graphics
	{
		Light::Light(const maths::Vector3& direction, const maths::Vector4& colour, float intensity, const LightType& type, const maths::Vector3& position, float radius)
			: m_Direction(direction), m_Colour(colour), m_Position(position), m_Intensity(intensity), m_Radius(radius), m_Type(float(type))
		{
		}
	}
}

