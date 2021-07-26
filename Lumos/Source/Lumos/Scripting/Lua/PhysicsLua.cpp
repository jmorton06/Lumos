#include "Precompiled.h"
#include "PhysicsLua.h"
#include "Scene/Component/Physics2DComponent.h"
#include "Scene/Component/Physics3DComponent.h"
#include "Core/Application.h"
#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"

#include <box2d/box2d.h>
#include <sol/sol.hpp>

namespace Lumos
{
    void register_type_b2Vec2(sol::state& state)
    {
        auto b2Vec2_addition_overloads = sol::overload(
            [](b2Vec2& left, const b2Vec2& right)
            {
                b2Vec2 ret = left;
                ret += right;
                return ret;
            });
        auto b2Vec2_subtraction_overloads = sol::overload(
            [](b2Vec2& left, const b2Vec2& right)
            {
                b2Vec2 ret = left;
                ret -= right;
                return ret;
            });
        auto b2Vec2_multiplication_overloads = sol::overload(
            [](const float& left, b2Vec2& right)
            {
                b2Vec2 ret = right;
                ret *= left;
                return ret;
            },
            [](b2Vec2& left, const float& right)
            {
                b2Vec2 ret = left;
                ret *= right;
                return ret;
            });
        state.new_usertype<b2Vec2>("b2Vec2"
            // fields
            ,
            "x",
            &b2Vec2::x,
            "y",
            &b2Vec2::y
            // methods
            ,
            "SetZero",
            &b2Vec2::SetZero,
            "Set",
            &b2Vec2::Set,
            "Length",
            &b2Vec2::Length,
            "LengthSquared",
            &b2Vec2::LengthSquared,
            "Normalise",
            &b2Vec2::Normalize,
            "IsValid",
            &b2Vec2::IsValid,
            "Skew",
            &b2Vec2::Skew
            // constructors
            ,
            sol::call_constructor,
            sol::constructors<
                b2Vec2(),
                b2Vec2(float, float)>()
            // metas
            ,
            sol::meta_function::addition,
            b2Vec2_addition_overloads,
            sol::meta_function::subtraction,
            b2Vec2_subtraction_overloads,
            sol::meta_function::multiplication,
            b2Vec2_multiplication_overloads);
    }
    void register_type_b2Vec3(sol::state& state)
    {
        auto b2Vec3_addition_overloads = sol::overload(
            [](b2Vec3& left, const b2Vec3& right)
            {
                b2Vec3 ret = left;
                ret += right;
                return ret;
            });
        auto b2Vec3_subtraction_overloads = sol::overload(
            [](b2Vec3& left, const b2Vec3& right)
            {
                b2Vec3 ret = left;
                ret -= right;
                return ret;
            });
        auto b2Vec3_multiplication_overloads = sol::overload(
            [](const float& left, b2Vec3& right)
            {
                b2Vec3 ret = right;
                ret *= left;
                return ret;
            },
            [](b2Vec3& left, const float& right)
            {
                b2Vec3 ret = left;
                ret *= right;
                return ret;
            });
        state.new_usertype<b2Vec3>("b2Vec3"
            // fields
            ,
            "x",
            &b2Vec3::x,
            "y",
            &b2Vec3::y,
            "z",
            &b2Vec3::z
            // methods
            ,
            "SetZero",
            &b2Vec3::SetZero,
            "Set",
            &b2Vec3::Set
            // constructors
            ,
            sol::call_constructor,
            sol::constructors<
                b2Vec3(),
                b2Vec3(float, float, float)>()
            // metas
            ,
            sol::meta_function::addition,
            b2Vec3_addition_overloads,
            sol::meta_function::subtraction,
            b2Vec3_subtraction_overloads,
            sol::meta_function::multiplication,
            b2Vec3_multiplication_overloads);
    }
    void register_type_b2Mat22(sol::state& state)
    {
        state.new_usertype<b2Mat22>("b2Mat22"
            // fields
            ,
            "ex",
            &b2Mat22::ex,
            "ey",
            &b2Mat22::ey
            // methods
            ,
            "Set",
            &b2Mat22::Set,
            "SetIdentity",
            &b2Mat22::SetIdentity,
            "SetZero",
            &b2Mat22::SetZero,
            "GetInverse",
            &b2Mat22::GetInverse,
            "Solve",
            &b2Mat22::Solve
            // constructors
            ,
            sol::call_constructor,
            sol::constructors<
                b2Mat22(),
                b2Mat22(const b2Vec2&, const b2Vec2&),
                b2Mat22(float, float, float, float)>());
    }
    void register_type_b2Mat33(sol::state& state)
    {
        state.new_usertype<b2Mat33>("b2Mat33"
            // fields
            ,
            "ex",
            &b2Mat33::ex,
            "ey",
            &b2Mat33::ey,
            "ez",
            &b2Mat33::ez
            // methods
            ,
            "SetZero",
            &b2Mat33::SetZero,
            "Solve33",
            &b2Mat33::Solve33,
            "Solve22",
            &b2Mat33::Solve22,
            "GetInverse22",
            &b2Mat33::GetInverse22,
            "GetSymInverse33",
            &b2Mat33::GetSymInverse33
            // constructors
            ,
            sol::call_constructor,
            sol::constructors<
                b2Mat33(),
                b2Mat33(const b2Vec3&, const b2Vec3&, const b2Vec3&)>());
    }
    void register_type_b2Rot(sol::state& state)
    {
        state.new_usertype<b2Rot>("b2Rot"
            // fields
            ,
            "s",
            &b2Rot::s,
            "c",
            &b2Rot::c
            // methods
            ,
            "Set",
            &b2Rot::Set,
            "SetIdentity",
            &b2Rot::SetIdentity,
            "GetAngle",
            &b2Rot::GetAngle,
            "GetXAxis",
            &b2Rot::GetXAxis,
            "GetYAxis",
            &b2Rot::GetYAxis
            // constructors
            ,
            sol::call_constructor,
            sol::constructors<
                b2Rot(),
                b2Rot(float)>());
    }
    void register_type_b2Transform(sol::state& state)
    {
        state.new_usertype<b2Transform>("b2Transform"
            // fields
            ,
            "p",
            &b2Transform::p,
            "q",
            &b2Transform::q
            // methods
            ,
            "SetIdentity",
            &b2Transform::SetIdentity,
            "Set",
            &b2Transform::Set
            // constructors
            ,
            sol::call_constructor,
            sol::constructors<
                b2Transform(),
                b2Transform(const b2Vec2&, const b2Rot&)>());
    }
    void register_type_b2Sweep(sol::state& state)
    {
        state.new_usertype<b2Sweep>("b2Sweep"
            // fields
            ,
            "localCenter",
            &b2Sweep::localCenter,
            "c0",
            &b2Sweep::c0,
            "c",
            &b2Sweep::c,
            "a0",
            &b2Sweep::a0,
            "a",
            &b2Sweep::a,
            "alpha0",
            &b2Sweep::alpha0
            // methods
            ,
            "GetTransform",
            &b2Sweep::GetTransform,
            "Advance",
            &b2Sweep::Advance,
            "Normalise",
            &b2Sweep::Normalize
            // constructors
        );
    }

    class ContactListener : public b2ContactListener
    {
    public:
        void BeginContact(b2Contact* contact)
        {
            beginContactFunction(contact->GetFixtureA()->GetBody(), contact->GetFixtureB()->GetBody());
        }

        void EndContact(b2Contact* contact)
        {
        }

        void SetBeginFunction(sol::function func)
        {
            beginContactFunction = func;
        };

    private:
        sol::function beginContactFunction;
    };

    static void SetCallback(const sol::function& func)
    {
        ContactListener* listener = new ContactListener();
        listener->SetBeginFunction(func);
        Application::Get().GetSystem<B2PhysicsEngine>()->SetContactListener(listener);
    }

    static void SetB2DGravity(const Maths::Vector2& gravity)
    {
        Application::Get().GetSystem<B2PhysicsEngine>()->SetGravity(gravity);
    }

    SharedRef<RigidBody3D> CreateSharedPhysics3D()
    {
        return CreateSharedRef<RigidBody3D>();
    }

    SharedRef<RigidBody3D> CreateSharedPhysics3DWithParams(const RigidBodyParameters& params)
    {
        return CreateSharedRef<RigidBody3D>();
    }

    void BindPhysicsLua(sol::state& state)
    {
        register_type_b2Vec2(state);
        register_type_b2Vec3(state);
        register_type_b2Mat22(state);
        register_type_b2Mat33(state);
        register_type_b2Rot(state);
        register_type_b2Transform(state);
        register_type_b2Sweep(state);

        sol::usertype<RigidBodyParameters> physicsObjectParameters_type = state.new_usertype<RigidBodyParameters>("RigidBodyParameters");
        physicsObjectParameters_type["mass"] = &RigidBodyParameters::mass;
        physicsObjectParameters_type["shape"] = &RigidBodyParameters::shape;
        physicsObjectParameters_type["position"] = &RigidBodyParameters::position;
        physicsObjectParameters_type["scale"] = &RigidBodyParameters::scale;
        physicsObjectParameters_type["isStatic"] = &RigidBodyParameters::isStatic;
        physicsObjectParameters_type["customShapePositions"] = &RigidBodyParameters::custumShapePositions;

        sol::usertype<RigidBody3D> physics3D_type = state.new_usertype<RigidBody3D>("RigidBody3D", sol::constructors<RigidBody2D>()); //;const RigidBodyParameters&)>());
        physics3D_type.set_function("SetForce", &RigidBody3D::SetForce);
        physics3D_type.set_function("SetPosition", &RigidBody3D::SetPosition);
        physics3D_type.set_function("SetLinearVelocity", &RigidBody3D::SetLinearVelocity);
        physics3D_type.set_function("SetOrientation", &RigidBody3D::SetOrientation);
        physics3D_type.set_function("SetAngularVelocity", &RigidBody3D::SetAngularVelocity);
        physics3D_type.set_function("SetFriction", &RigidBody3D::SetFriction);
        physics3D_type.set_function("GetPosition", &RigidBody3D::GetPosition);
        physics3D_type.set_function("GetFriction", &RigidBody3D::GetFriction);
        physics3D_type.set_function("GetIsStatic", &RigidBody3D::GetIsStatic);

        std::initializer_list<std::pair<sol::string_view, Shape>> shapes = {
            { "Square", Shape::Square },
            { "Circle", Shape::Circle },
            { "Custom", Shape::Custom }
        };
        state.new_enum<Shape, false>("Shape", shapes);

        sol::usertype<RigidBody2D> physics2D_type = state.new_usertype<RigidBody2D>("RigidBody2D", sol::constructors<RigidBody2D(const RigidBodyParameters&)>());
        physics2D_type.set_function("SetForce", &RigidBody2D::SetForce);
        physics2D_type.set_function("SetPosition", &RigidBody2D::SetPosition);
        physics2D_type.set_function("SetLinearVelocity", &RigidBody2D::SetLinearVelocity);
        physics2D_type.set_function("SetOrientation", &RigidBody2D::SetOrientation);
        physics2D_type.set_function("SetAngularVelocity", &RigidBody2D::SetAngularVelocity);
        physics2D_type.set_function("SetFriction", &RigidBody2D::SetFriction);
        physics2D_type.set_function("GetLinearVelocity", &RigidBody2D::GetLinearVelocity);

        physics2D_type.set_function("GetPosition", &RigidBody2D::GetPosition);
        physics2D_type.set_function("GetAngle", &RigidBody2D::GetAngle);
        physics2D_type.set_function("GetFriction", &RigidBody2D::GetFriction);
        physics2D_type.set_function("GetIsStatic", &RigidBody2D::GetIsStatic);
        physics2D_type.set_function("GetB2Body", &RigidBody2D::GetB2BodyRef);
        physics2D_type.set_function("Init", &RigidBody2D::Init);
        physics2D_type.set_function("SetIsStatic", &RigidBody2D::SetIsStatic);

        state.set_function("SetCallback", &SetCallback);
        state.set_function("SetB2DGravity", &SetB2DGravity);

        state.new_enum("b2BodyType", "b2_staticBody", b2BodyType::b2_staticBody, "b2_kinematicBody", b2BodyType::b2_kinematicBody, "b2_dynamicBody", b2BodyType::b2_dynamicBody);

        state.new_usertype<b2BodyDef>("b2BodyDef"
            // fields
            ,
            "type",
            &b2BodyDef::type,
            "position",
            &b2BodyDef::position,
            "angle",
            &b2BodyDef::angle,
            "linearVelocity",
            &b2BodyDef::linearVelocity,
            "angularVelocity",
            &b2BodyDef::angularVelocity,
            "linearDamping",
            &b2BodyDef::linearDamping,
            "angularDamping",
            &b2BodyDef::angularDamping,
            "allowSleep",
            &b2BodyDef::allowSleep,
            "awake",
            &b2BodyDef::awake,
            "fixedRotation",
            &b2BodyDef::fixedRotation,
            "bullet",
            &b2BodyDef::bullet,
            "userData",
            &b2BodyDef::userData,
            "gravityScale",
            &b2BodyDef::gravityScale
            // methods
            // constructors
            ,
            sol::call_constructor,
            sol::constructors<
                b2BodyDef()>());

        state.new_usertype<b2Body>("b2Body", "CreateFixture", sol::overload(sol::resolve<b2Fixture*(const b2FixtureDef*)>(&b2Body::CreateFixture), sol::resolve<b2Fixture*(const b2Shape*, float)>(&b2Body::CreateFixture)), "DestroyFixture", &b2Body::DestroyFixture, "SetTransform", &b2Body::SetTransform, "GetTransform", &b2Body::GetTransform, "GetPosition", &b2Body::GetPosition, "GetAngle", &b2Body::GetAngle, "GetWorldCenter", &b2Body::GetWorldCenter, "GetLocalCenter", &b2Body::GetLocalCenter, "SetLinearVelocity", &b2Body::SetLinearVelocity, "GetLinearVelocity", &b2Body::GetLinearVelocity, "SetAngularVelocity", &b2Body::SetAngularVelocity, "GetAngularVelocity", &b2Body::GetAngularVelocity, "ApplyForce", &b2Body::ApplyForce, "ApplyForceToCenter", &b2Body::ApplyForceToCenter, "ApplyTorque", &b2Body::ApplyTorque, "ApplyLinearImpulse", &b2Body::ApplyLinearImpulse, "ApplyAngularImpulse", &b2Body::ApplyAngularImpulse, "GetMass", &b2Body::GetMass, "GetInertia", &b2Body::GetInertia, "GetMassData", &b2Body::GetMassData, "SetMassData", &b2Body::SetMassData, "ResetMassData", &b2Body::ResetMassData, "GetWorldPoint", &b2Body::GetWorldPoint, "GetWorldVector", &b2Body::GetWorldVector, "GetLocalPoint", &b2Body::GetLocalPoint, "GetLocalVector", &b2Body::GetLocalVector, "GetLinearVelocityFromWorldPoint", &b2Body::GetLinearVelocityFromWorldPoint, "GetLinearVelocityFromLocalPoint", &b2Body::GetLinearVelocityFromLocalPoint, "GetLinearDamping", &b2Body::GetLinearDamping, "SetLinearDamping", &b2Body::SetLinearDamping, "GetAngularDamping", &b2Body::GetAngularDamping, "SetAngularDamping", &b2Body::SetAngularDamping, "GetGravityScale", &b2Body::GetGravityScale, "SetGravityScale", &b2Body::SetGravityScale, "SetType", &b2Body::SetType, "GetType", &b2Body::GetType, "SetBullet", &b2Body::SetBullet, "IsBullet", &b2Body::IsBullet, "SetSleepingAllowed", &b2Body::SetSleepingAllowed, "IsSleepingAllowed", &b2Body::IsSleepingAllowed, "SetAwake", &b2Body::SetAwake, "IsAwake", &b2Body::IsAwake, "SetFixedRotation", &b2Body::SetFixedRotation, "IsFixedRotation", &b2Body::IsFixedRotation, "GetFixtureList", sol::overload(sol::resolve<b2Fixture*()>(&b2Body::GetFixtureList), sol::resolve<const b2Fixture*() const>(&b2Body::GetFixtureList)), "GetJointList", sol::overload(sol::resolve<b2JointEdge*()>(&b2Body::GetJointList), sol::resolve<const b2JointEdge*() const>(&b2Body::GetJointList)), "GetContactList", sol::overload(sol::resolve<b2ContactEdge*()>(&b2Body::GetContactList), sol::resolve<const b2ContactEdge*() const>(&b2Body::GetContactList)), "GetNext", sol::overload(sol::resolve<b2Body*()>(&b2Body::GetNext), sol::resolve<const b2Body*() const>(&b2Body::GetNext)), "GetUserData", &b2Body::GetUserData, "SetUserData", &b2Body::SetUserData, "GetWorld", sol::overload(sol::resolve<b2World*()>(&b2Body::GetWorld), sol::resolve<const b2World*() const>(&b2Body::GetWorld)), "Dump", &b2Body::Dump
            // constructors
        );
    }
}
