#pragma once

#include "Utilities/TSingleton.h"
#include "Scene/ISystem.h"

class b2World;
class b2Body;
struct b2BodyDef;
struct b2FixtureDef;
class b2ContactListener;

namespace Lumos
{
    class TimeStep;
    class B2DebugDraw;

    class LUMOS_EXPORT B2PhysicsEngine : public ISystem
    {
        friend class TSingleton<B2PhysicsEngine>;

    public:
        B2PhysicsEngine();
        ~B2PhysicsEngine();
        void SetDefaults();

        void OnUpdate(const TimeStep& timeStep, Scene* scene) override;
        void OnInit() override {};
        void OnImGui() override;

        b2World* GetB2World() const
        {
            return m_B2DWorld.get();
        }
        b2Body* CreateB2Body(b2BodyDef* bodyDef) const;

        static void CreateFixture(b2Body* body, const b2FixtureDef* fixtureDef);

        void SetPaused(bool paused)
        {
            m_Paused = paused;
        }
        bool IsPaused() const
        {
            return m_Paused;
        }

        void OnDebugDraw() override;

        uint32_t GetDebugDrawFlags();
        void SetDebugDrawFlags(uint32_t flags);
        void SetGravity(const Maths::Vector2& gravity);

        void SetContactListener(b2ContactListener* listener);

    private:
        UniqueRef<b2World> m_B2DWorld;
        UniqueRef<B2DebugDraw> m_DebugDraw;

        float m_UpdateTimestep, m_UpdateAccum;
        bool m_Paused = true;
        bool m_MultipleUpdates = true;

        b2ContactListener* m_Listener;
    };
}
