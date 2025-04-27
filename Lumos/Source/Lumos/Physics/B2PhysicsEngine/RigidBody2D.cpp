#include "Precompiled.h"
#include "RigidBody2D.h"
#include "B2PhysicsEngine.h"
#include "Core/Application.h"

#include "Maths/Vector2.h"
#include <box2d/box2d.h>

namespace Lumos
{

    RigidBody2D::RigidBody2D()
    {
        Init(RigidBodyParameters());
    }

    RigidBody2D::RigidBody2D(const RigidBodyParameters& params)
    {
        Init(params);
    }

    RigidBody2D::~RigidBody2D()
    {
        ASSERT(Application::Get().GetSystemManager() && Application::Get().GetSystem<B2PhysicsEngine>());

        ContactCallback* callback = (ContactCallback*)b2Body_GetUserData(m_B2Body);
        if(callback)
            delete callback;
        b2DestroyBody(m_B2Body);
    }

    void RigidBody2D::SetLinearVelocity(const Vec2& v) const
    {
        b2Body_SetLinearVelocity(m_B2Body, { v.x, v.y });
    }

    void RigidBody2D::SetAngularVelocity(float velocity)
    {
        b2Body_SetAngularVelocity(m_B2Body, velocity);
    }

    void RigidBody2D::SetForce(const Vec2& v) const
    {
        b2Body_ApplyForceToCenter(m_B2Body, { v.x, v.y }, true);
    }

    void RigidBody2D::SetPosition(const Vec2& pos) const
    {
        b2Body_SetTransform(m_B2Body, { pos.x, pos.y }, b2Body_GetRotation(m_B2Body));
    }

    void RigidBody2D::SetOrientation(float angle) const
    {
        b2Body_SetTransform(m_B2Body, b2Body_GetPosition(m_B2Body), b2MakeRot(angle));
    }

    void RigidBody2D::SetIsStatic(bool isStatic)
    {
        m_Static = isStatic;
        b2Body_SetType(m_B2Body, isStatic ? b2_staticBody : b2_dynamicBody);
    }

    void RigidBody2D::Init(const RigidBodyParameters& params)
    {
        LUMOS_PROFILE_FUNCTION();
        m_Static    = params.isStatic;
        m_ShapeType = params.shape;
        m_Mass      = params.mass;
        m_Scale     = params.scale;
        m_Friction  = params.friction;
		
		LINFO("%.2f friction set", m_Friction);
			  

        b2BodyDef bodyDef = b2DefaultBodyDef();
        if(params.isStatic)
            bodyDef.type = b2_staticBody;
        else
            bodyDef.type = b2_dynamicBody;

        bodyDef.linearDamping = 1.0f;
        bodyDef.position      = { params.position.x, params.position.y };

        b2WorldId lWorldID = Application::Get().GetSystem<B2PhysicsEngine>()->GetB2World();
        m_B2Body           = b2CreateBody(lWorldID, &bodyDef);

        if(params.shape == Shape::Circle)
        {
            b2Circle circle = { { 0.0f, 0.0f }, params.scale.x };

            b2ShapeDef shapeDef      = b2DefaultShapeDef();
            shapeDef.density         = 1.0f;
            shapeDef.friction        = m_Friction;
            shapeDef.enableHitEvents = true;

            b2CreateCircleShape(m_B2Body, &shapeDef, &circle);
        }
        else if(params.shape == Shape::Square)
        {
            b2Polygon box = b2MakeBox(params.scale.x, params.scale.y);

            b2ShapeDef shapeDef      = b2DefaultShapeDef();
            shapeDef.density         = 1.0f;
            shapeDef.friction        = m_Friction;
            shapeDef.enableHitEvents = true;

            b2CreatePolygonShape(m_B2Body, &shapeDef, &box);
        }
        else if(params.shape == Shape::Custom)
        {
            m_CustomShapePositions = params.customShapePositions;

            ArenaTemp temp           = ScratchBegin(0, 0);
            b2Vec2* b2ShapePositions = PushArrayNoZero(temp.arena, b2Vec2, i32(params.customShapePositions.size()));
            for(i32 i = 0; i < i32(params.customShapePositions.size()); i++)
            {
                b2ShapePositions[i].x = m_CustomShapePositions[i].x;
                b2ShapePositions[i].y = m_CustomShapePositions[i].y;
            }

            b2Hull hull             = b2ComputeHull(b2ShapePositions, i32(params.customShapePositions.size()));
            b2Polygon customPolygon = b2MakePolygon(&hull, 0.0f);

            b2ShapeDef shapeDef      = b2DefaultShapeDef();
            shapeDef.density         = 1.0f;
            shapeDef.friction        = m_Friction;
            shapeDef.enableHitEvents = true;

            b2CreatePolygonShape(m_B2Body, &shapeDef, &customPolygon);
            ScratchEnd(temp);
        }
        else
        {
            LERROR("Shape Not Supported");
        }
    }

    Vec2 RigidBody2D::GetPosition() const
    {
        b2Vec2 pos = b2Body_GetPosition(m_B2Body);
        return Vec2(pos.x, pos.y);
    }

    float RigidBody2D::GetAngle() const
    {
        return b2Rot_GetAngle(b2Body_GetRotation(m_B2Body));
    }

    const Vec2 RigidBody2D::GetLinearVelocity() const
    {
        b2Vec2 vel = b2Body_GetLinearVelocity(m_B2Body);
        return Vec2(vel.x, vel.y);
    }

    void RigidBody2D::RebuildShape()
    {
        b2DestroyBody(m_B2Body);

        RigidBodyParameters params;
        // params.position = m_Position;
        params.scale    = m_Scale;
        params.shape    = m_ShapeType;
        params.mass     = m_Mass;
        params.scale    = m_Scale;
        params.friction = m_Friction;

        Init(params);
    }

    void RigidBody2D::SetLinearDamping(float dampening)
    {
        b2Body_SetLinearDamping(m_B2Body, dampening);
    }

    void RigidBody2D::SetShape(Shape shape, const std::vector<Vec2>& customPositions)
    {
        LUMOS_PROFILE_FUNCTION();
        m_ShapeType            = shape;
        m_CustomShapePositions = customPositions;

        RigidBodyParameters params;
        params.shape                = m_ShapeType;
        params.position             = Vec3(GetPosition(), 1.0f);
        params.customShapePositions = customPositions;
        params.mass                 = m_Mass;
        params.scale                = m_Scale;
        params.isStatic             = m_Static;

        b2DestroyBody(m_B2Body);
        Init(params);
    }

    float RigidBody2D::GetFriction() const
    {
        return m_Friction;
    }
}
