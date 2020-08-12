#pragma once
#include "lmpch.h"
#include "Physics/B2PhysicsEngine/RigidBody2D.h"

#include <cereal/cereal.hpp>

namespace Lumos
{
	class LUMOS_EXPORT Physics2DComponent
	{
	public:
		Physics2DComponent();
		Physics2DComponent(const RigidBodyParameters& params);
		explicit Physics2DComponent(Ref<RigidBody2D>& physics);

		void Update();
		void OnImGui();

		RigidBody2D* GetRigidBodyRaw()
		{
			return m_RigidBody.get();
		}
		Ref<RigidBody2D> GetRigidBody()
		{
			return m_RigidBody;
		}

		template<typename Archive>
		void save(Archive& archive) const
		{
			archive(*m_RigidBody.get());
		}

		template<typename Archive>
		void load(Archive& archive)
		{
			m_RigidBody = CreateRef<RigidBody2D>();
			archive(*m_RigidBody.get());
		}

	private:
		Ref<RigidBody2D> m_RigidBody;
	};
}
