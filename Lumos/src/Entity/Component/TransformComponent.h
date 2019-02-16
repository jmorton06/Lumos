#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "Maths/Matrix4.h"

namespace Lumos
{
	class LUMOS_EXPORT TransformComponent : public LumosComponent
	{
	public:
		maths::Matrix4 m_WorldSpaceTransform;
		maths::Matrix4 m_LocalTransform;
	public:
		explicit TransformComponent(const maths::Matrix4& matrix);

		void SetBothTransforms(const maths::Matrix4& matrix) { m_LocalTransform = matrix; m_WorldSpaceTransform = matrix; }
		static ComponentType GetStaticType()
		{
            static ComponentType type(ComponentType::Transform);
			return type;
		}

		inline virtual ComponentType GetType() const override { return GetStaticType(); }
        void OnIMGUI() override;
	};
}
