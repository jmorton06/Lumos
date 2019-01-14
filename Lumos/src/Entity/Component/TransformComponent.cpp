#include "LM.h"
#include "TransformComponent.h"

namespace Lumos
{
	TransformComponent::TransformComponent(const maths::Matrix4& matrix)
		: m_WorldSpaceTransform(matrix) , m_LocalTransform(matrix)
	{

	}

}
