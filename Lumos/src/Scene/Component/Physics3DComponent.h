#pragma once
#include "lmpch.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"

#include <cereal/cereal.hpp>

namespace Lumos
{
	class LUMOS_EXPORT Physics3DComponent
	{
	public:
		Physics3DComponent();
		explicit Physics3DComponent(Ref<RigidBody3D>& physics);

		~Physics3DComponent() = default;

		void Init();
		void Update();
		void OnImGui();

		const Ref<RigidBody3D>& GetRigidBody() const
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
			m_RigidBody = CreateRef<RigidBody3D>();
			archive(*m_RigidBody.get());
		}

	private:
		Ref<RigidBody3D> m_RigidBody;
	};
}
