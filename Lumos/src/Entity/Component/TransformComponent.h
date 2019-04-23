#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "Maths/Transform.h"

namespace Lumos
{
	class LUMOS_EXPORT TransformComponent : public LumosComponent
	{
	public:
		explicit TransformComponent(const maths::Matrix4& matrix);

        void SetWorldMatrix(const maths::Matrix4& matrix);
		static ComponentType GetStaticType()
		{
            static ComponentType type(ComponentType::Transform);
			return type;
		}

		inline virtual ComponentType GetType() const override { return GetStaticType(); }
		void OnUpdateComponent(float dt) override;
        void OnIMGUI() override;
    public:
		maths::Transform m_Transform;
	};
}
