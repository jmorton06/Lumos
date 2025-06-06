#pragma once

#include "Physics/B2PhysicsEngine/RigidBody2D.h"

#include <cereal/cereal.hpp>

namespace Lumos
{
    class LUMOS_EXPORT RigidBody2DComponent
    {
    public:
        RigidBody2DComponent();
        RigidBody2DComponent(const RigidBodyParameters& params);
        explicit RigidBody2DComponent(SharedPtr<RigidBody2D>& physics);

        void Update();
        void OnImGui();

        RigidBody2D* GetRigidBodyRaw()
        {
            return m_RigidBody.get();
        }
        SharedPtr<RigidBody2D> GetRigidBody()
        {
            return m_RigidBody;
        }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(*m_RigidBody.get());
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            m_RigidBody = CreateSharedPtr<RigidBody2D>();
            archive(*m_RigidBody.get());
        }

    private:
        SharedPtr<RigidBody2D> m_RigidBody;
    };
}
