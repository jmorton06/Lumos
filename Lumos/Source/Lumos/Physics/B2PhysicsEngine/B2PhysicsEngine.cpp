#include "Precompiled.h"
#include "B2PhysicsEngine.h"
#include "RigidBody2D.h"

#include "Utilities/TimeStep.h"

#include "Scene/Component/RigidBody2DComponent.h"
#include "Scene/Scene.h"

#include "Maths/Transform.h"
#include "Maths/MathsUtilities.h"
#include "B2DebugDraw.h"

#include <box2d/box2d.h>

#include <imgui/imgui.h>
#include <entt/entity/registry.hpp>

namespace Lumos
{
    B2PhysicsEngine::B2PhysicsEngine()
        : m_UpdateTimestep(1.0f / 60.f)
        , m_Paused(false)
    {
        m_DebugName = "Box2D Physics Engine";

        b2Vec2 gravity      = { 0.0f, -9.81f };
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity    = gravity;
        m_B2DWorld          = b2CreateWorld(&worldDef);

        b2AABB bounds = { { -FLT_MAX, -FLT_MAX }, { FLT_MAX, FLT_MAX } };

        m_DebugDraw = { B2DebugDraw::DrawPolygon,
                        B2DebugDraw::DrawSolidPolygon,
                        B2DebugDraw::DrawCircle,
                        B2DebugDraw::DrawSolidCircle,
                        0, // capsule
                        B2DebugDraw::DrawSegment,
                        B2DebugDraw::DrawTransform,
                        B2DebugDraw::DrawPoint,
                        B2DebugDraw::DrawString, // Draw String
                        bounds,
                        true,  // drawUsingBounds
                        true,  // shapes
                        false, // joints
                        false, // joint extras
                        false, // aabbs
                        false, // mass
                        true,  // draw names
                        false, // contacts
                        false, // colors
                        false, // normals
                        false, // impulse
                        false, // features
                        false, // friction
                        false, // islands
                        this };
    }

    B2PhysicsEngine::~B2PhysicsEngine()
    {
    }

    void B2PhysicsEngine::SetDefaults()
    {
        m_UpdateTimestep = 1.0f / 60.f;
    }

    void B2PhysicsEngine::OnUpdate(const TimeStep& timeStep, Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();

        if(!m_Paused)
        {
            b2World_Step(m_B2DWorld, (float)timeStep.GetSeconds(), 4);

            b2ContactEvents contactEvents = b2World_GetContactEvents(m_B2DWorld);
            for(int i = 0; i < contactEvents.beginCount; ++i)
            {
                b2ContactBeginTouchEvent event = contactEvents.beginEvents[i];
                b2BodyId bodyIdA               = b2Shape_GetBody(event.shapeIdA);
                b2BodyId bodyIdB               = b2Shape_GetBody(event.shapeIdB);

                ContactCallback* callbackA = (ContactCallback*)b2Body_GetUserData(bodyIdA);
                if(callbackA)
                {
                    callbackA->OnCollision(bodyIdA, bodyIdB, 1.0f); // event.approachSpeed);
                }
                ContactCallback* callbackB = (ContactCallback*)b2Body_GetUserData(bodyIdB);
                if(callbackB)
                {
                    callbackB->OnCollision(bodyIdB, bodyIdA, 1.0f); // event.approachSpeed);
                }
            }
        }
    }

    void B2PhysicsEngine::OnImGui()
    {
        ImGui::TextUnformatted("2D Physics Engine");

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Number Of Collision Pairs");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Text("%i", b2World_GetContactEvents(m_B2DWorld).hitCount);
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Number Of Rigid Bodys");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        // ImGui::Text("%5.2i", m_B2DWorld->GetBodyCount());
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Paused");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Checkbox("##Paused", &m_Paused);
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Gravity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        // float grav[2] = { m_B2DWorld->GetGravity().x, m_B2DWorld->GetGravity().y };
        // if(ImGui::InputFloat2("##Gravity", grav))
        //    m_B2DWorld->SetGravity({ grav[0], grav[1] });
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    b2BodyId B2PhysicsEngine::CreateB2Body(b2BodyDef bodyDef) const
    {
        b2BodyId bodyId = b2CreateBody(m_B2DWorld, &bodyDef);
        return bodyId;
    }

    void B2PhysicsEngine::OnDebugDraw()
    {
        if(m_DebugDrawFlags > 0)
            b2World_Draw(m_B2DWorld, &m_DebugDraw);
    }

    void B2PhysicsEngine::SetDebugDrawFlags(uint32_t flags)
    {
        m_DebugDrawFlags = flags;
        // m_DebugDraw.drawShapes = true;
        m_DebugDraw.drawJoints           = m_DebugDrawFlags & PhysicsDebugFlags2D::CONSTRAINT2D;
        m_DebugDraw.drawJointExtras      = m_DebugDrawFlags & PhysicsDebugFlags2D::CONSTRAINT2D;
        m_DebugDraw.drawBounds           = m_DebugDrawFlags & PhysicsDebugFlags2D::AABB2D;
        m_DebugDraw.drawMass             = m_DebugDrawFlags & PhysicsDebugFlags2D::AABB2D;
        m_DebugDraw.drawContacts         = m_DebugDrawFlags & PhysicsDebugFlags2D::AABB2D;
        m_DebugDraw.drawContactNormals   = m_DebugDrawFlags & PhysicsDebugFlags2D::COLLISIONNORMALS2D;
        m_DebugDraw.drawContactImpulses  = m_DebugDrawFlags & PhysicsDebugFlags2D::MANIFOLD2D;
        m_DebugDraw.drawFrictionImpulses = m_DebugDrawFlags & PhysicsDebugFlags2D::MANIFOLD2D;
    }

    uint32_t B2PhysicsEngine::GetDebugDrawFlags()
    {
        return m_DebugDrawFlags;
    }

    void B2PhysicsEngine::SetGravity(const Vec2& gravity)
    {
        b2World_SetGravity(m_B2DWorld, { gravity.x, gravity.y });
    }

    void B2PhysicsEngine::SyncTransforms(Scene* scene)
    {
        if(!scene)
            return;

        auto& registry = scene->GetRegistry();

        auto group = registry.group<RigidBody2DComponent>(entt::get<Maths::Transform>);

        for(auto entity : group)
        {
            const auto& [phys, trans] = group.get<RigidBody2DComponent, Maths::Transform>(entity);

            // if (!phys.GetRigidBody()->GetB2Body()->IsAwake())
            //     break;

            trans.SetLocalPosition(Vec3(phys.GetRigidBody()->GetPosition(), trans.GetLocalPosition().z));
            trans.SetLocalOrientation(Quat(Vec3(0.0f, 0.0f, Maths::ToDegrees(phys.GetRigidBody()->GetAngle()))));
        };
    }
}
