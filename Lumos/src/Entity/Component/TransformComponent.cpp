#include "JM.h"
#include "TransformComponent.h"

namespace jm
{
	TransformComponent::TransformComponent(const maths::Matrix4& matrix)
		: m_WorldSpaceTransform(matrix) , m_LocalTransform(matrix)
	{

	}

}
