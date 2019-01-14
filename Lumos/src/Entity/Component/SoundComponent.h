#pragma once
#include "JM.h"
#include "JMComponent.h"

namespace jm
{
	class SoundNode;

	class JM_EXPORT SoundComponent : public JMComponent
	{
	public:
		std::shared_ptr<SoundNode> m_SoundNode;
	public:
		explicit SoundComponent(std::shared_ptr<SoundNode>& sound);

		static ComponentType GetStaticType()
		{
			static ComponentType type(ComponentType::Sound);
			return type;
		}

		void OnUpdateComponent(float dt) override;
		void DebugDraw(uint64 debugFlags) override;
		void Init() override ;

		inline virtual ComponentType GetType() const override { return GetStaticType(); }
	};
}
