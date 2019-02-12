#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "Maths/Vector3.h"


namespace Lumos
{
	struct Light;

	class LUMOS_EXPORT LightComponent : public LumosComponent
	{
	public:
		std::shared_ptr<Light> m_Light;
		maths::Vector3 m_PostionOffset;
	public:
		explicit LightComponent(std::shared_ptr<Light>& light);

		static ComponentType GetStaticType()
		{
			static ComponentType type(ComponentType::Light);
			return type;
		}

		void SetPositionOffset(const maths::Vector3& vec) { m_PostionOffset = vec; };
		void SetRadius(float radius);

		void OnUpdateComponent(float dt) override;
		void Init() override;
		void DebugDraw(uint64 debugFlags) override;

		inline virtual ComponentType GetType() const override { return GetStaticType(); }
	};
}
