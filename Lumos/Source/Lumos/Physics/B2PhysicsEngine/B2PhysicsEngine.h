#pragma once

#include "Utilities/TSingleton.h"
#include "Scene/ISystem.h"
#include <box2d/id.h>
#include <box2d/types.h>

namespace Lumos
{
    class TimeStep;

    enum PhysicsDebugFlags2D : uint32_t
    {
        CONSTRAINT2D       = 1,
        MANIFOLD2D         = 2,
        COLLISIONVOLUMES2D = 4,
        COLLISIONNORMALS2D = 8,
        AABB2D             = 16,
        LINEARVELOCITY2D   = 32,
        LINEARFORCE2D      = 64
    };

    struct ContactCallback
    {
        virtual void OnCollision(b2BodyId a, b2BodyId b, float approachSpeed) { };
    };

    class LUMOS_EXPORT B2PhysicsEngine : public ISystem
    {
        friend class TSingleton<B2PhysicsEngine>;

    public:
        B2PhysicsEngine();
        ~B2PhysicsEngine();
        void SetDefaults();

        void OnUpdate(const TimeStep& timeStep, Scene* scene) override;
        bool OnInit() override { return true; };
        void OnImGui() override;

        b2WorldId GetB2World() const { return m_B2DWorld; }
        b2BodyId CreateB2Body(b2BodyDef bodyDef) const;

        void SetPaused(bool paused) { m_Paused = paused; }
        bool IsPaused() const { return m_Paused; }
        void OnDebugDraw() override;

        uint32_t GetDebugDrawFlags();
        void SetDebugDrawFlags(uint32_t flags);
        void SetGravity(const Vec2& gravity);

        void SyncTransforms(Scene* scene);

    private:
        b2WorldId m_B2DWorld;
        b2DebugDraw m_DebugDraw;

        u32 m_DebugDrawFlags = 0;

        float m_UpdateTimestep;
        bool m_Paused = true;

        int32_t m_VelocityIterations = 6;
        int32_t m_PositionIterations = 2;

        // b2ContactListener* m_Listener;
    };
}
