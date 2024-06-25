#include "Precompiled.h"
#include "PhysicsLua.h"
#include "Scene/Component/RigidBody2DComponent.h"
#include "Scene/Component/RigidBody3DComponent.h"
#include "Core/Application.h"
#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"

#include <box2d/box2d.h>
#include <sol/sol.hpp>

namespace Lumos
{
    void register_type_b2Vec2(sol::state& state)
    {
        auto b2Vec2type = state.new_usertype<b2Vec2>("b2Vec2", sol::constructors<b2Vec2(), b2Vec2(float, float)>());

        // Fields
        b2Vec2type["x"] = &b2Vec2::x;
        b2Vec2type["y"] = &b2Vec2::y;

        // Methods
        b2Vec2type["SetZero"]       = &b2Vec2::SetZero;
        b2Vec2type["Set"]           = &b2Vec2::Set;
        b2Vec2type["Length"]        = &b2Vec2::Length;
        b2Vec2type["LengthSquared"] = &b2Vec2::LengthSquared;
        b2Vec2type["Normalize"]     = &b2Vec2::Normalize;
        b2Vec2type["IsValid"]       = &b2Vec2::IsValid;
        b2Vec2type["Skew"]          = &b2Vec2::Skew;

        // Meta-functions
        b2Vec2type[sol::meta_function::addition] = [](b2Vec2& left, const b2Vec2& right)
        {
            b2Vec2 ret = left;
            ret += right;
            return ret;
        };

        b2Vec2type[sol::meta_function::subtraction] = [](b2Vec2& left, const b2Vec2& right)
        {
            b2Vec2 ret = left;
            ret -= right;
            return ret;
        };

        b2Vec2type[sol::meta_function::multiplication] = sol::overload(
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
    }
    void register_type_b2Vec3(sol::state& state)
    {
        auto b2Vec3type = state.new_usertype<b2Vec3>("b2Vec3", sol::constructors<b2Vec3(), b2Vec3(float, float, float)>());

        // Fields
        b2Vec3type["x"] = &b2Vec3::x;
        b2Vec3type["y"] = &b2Vec3::y;
        b2Vec3type["z"] = &b2Vec3::z;

        // Methods
        b2Vec3type["SetZero"] = &b2Vec3::SetZero;
        b2Vec3type["Set"]     = &b2Vec3::Set;

        // Meta-functions
        b2Vec3type[sol::meta_function::addition] = [](b2Vec3& left, const b2Vec3& right)
        {
            b2Vec3 ret = left;
            ret += right;
            return ret;
        };

        b2Vec3type[sol::meta_function::subtraction] = [](b2Vec3& left, const b2Vec3& right)
        {
            b2Vec3 ret = left;
            ret -= right;
            return ret;
        };

        b2Vec3type[sol::meta_function::multiplication] = sol::overload(
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
    }
    void register_type_b2Mat22(sol::state& state)
    {
        auto b2Mat22type = state.new_usertype<b2Mat22>("b2Mat22", sol::constructors<b2Mat22(), b2Mat22(const b2Vec2&, const b2Vec2&), b2Mat22(float, float, float, float)>());

        // Fields
        b2Mat22type["ex"] = &b2Mat22::ex;
        b2Mat22type["ey"] = &b2Mat22::ey;

        // Methods
        b2Mat22type["Set"]         = &b2Mat22::Set;
        b2Mat22type["SetIdentity"] = &b2Mat22::SetIdentity;
        b2Mat22type["SetZero"]     = &b2Mat22::SetZero;
        b2Mat22type["GetInverse"]  = &b2Mat22::GetInverse;
        b2Mat22type["Solve"]       = &b2Mat22::Solve;
    }
    void register_type_b2Mat33(sol::state& state)
    {
        auto b2Mat33type = state.new_usertype<b2Mat33>("b2Mat33", sol::constructors<b2Mat33(), b2Mat33(const b2Vec3&, const b2Vec3&, const b2Vec3&)>());

        // Fields
        b2Mat33type["ex"] = &b2Mat33::ex;
        b2Mat33type["ey"] = &b2Mat33::ey;
        b2Mat33type["ez"] = &b2Mat33::ez;

        // Methods
        b2Mat33type["SetZero"]         = &b2Mat33::SetZero;
        b2Mat33type["Solve33"]         = &b2Mat33::Solve33;
        b2Mat33type["Solve22"]         = &b2Mat33::Solve22;
        b2Mat33type["GetInverse22"]    = &b2Mat33::GetInverse22;
        b2Mat33type["GetSymInverse33"] = &b2Mat33::GetSymInverse33;
    }
    void register_type_b2Rot(sol::state& state)
    {
        auto b2Rottype = state.new_usertype<b2Rot>("b2Rot", sol::constructors<b2Rot(), b2Rot(float)>());

        // Fields
        b2Rottype["s"] = &b2Rot::s;
        b2Rottype["c"] = &b2Rot::c;

        // Methods
        b2Rottype["Set"]         = &b2Rot::Set;
        b2Rottype["SetIdentity"] = &b2Rot::SetIdentity;
        b2Rottype["GetAngle"]    = &b2Rot::GetAngle;
        b2Rottype["GetXAxis"]    = &b2Rot::GetXAxis;
        b2Rottype["GetYAxis"]    = &b2Rot::GetYAxis;
    }
    void register_type_b2Transform(sol::state& state)
    {
        auto b2Transformtype = state.new_usertype<b2Transform>("b2Transform");

        // Fields
        b2Transformtype["p"] = &b2Transform::p;
        b2Transformtype["q"] = &b2Transform::q;

        // Methods
        b2Transformtype["SetIdentity"] = &b2Transform::SetIdentity;
        b2Transformtype["Set"]         = &b2Transform::Set;

        // Constructors
        b2Transformtype[sol::call_constructor] = sol::constructors<
            b2Transform(),
            b2Transform(const b2Vec2&, const b2Rot&)>();
    }
    void register_type_b2Sweep(sol::state& state)
    {
        auto b2Sweeptype = state.new_usertype<b2Sweep>("b2Sweep");

        // Fields
        b2Sweeptype["localCenter"] = &b2Sweep::localCenter;
        b2Sweeptype["c0"]          = &b2Sweep::c0;
        b2Sweeptype["c"]           = &b2Sweep::c;
        b2Sweeptype["a0"]          = &b2Sweep::a0;
        b2Sweeptype["a"]           = &b2Sweep::a;
        b2Sweeptype["alpha0"]      = &b2Sweep::alpha0;

        // Methods
        b2Sweeptype["GetTransform"] = &b2Sweep::GetTransform;
        b2Sweeptype["Advance"]      = &b2Sweep::Advance;
        b2Sweeptype["Normalize"]    = &b2Sweep::Normalize;
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

    static void SetB2DGravity(const glm::vec2& gravity)
    {
        Application::Get().GetSystem<B2PhysicsEngine>()->SetGravity(gravity);
    }

    SharedPtr<RigidBody3D> CreateSharedPhysics3D()
    {
        return SharedPtr<RigidBody3D>(Application::Get().GetSystem<LumosPhysicsEngine>()->CreateBody({}));
    }

    SharedPtr<RigidBody3D> CreateSharedPhysics3DWithParams(const RigidBodyParameters& params)
    {
        return SharedPtr<RigidBody3D>(Application::Get().GetSystem<LumosPhysicsEngine>()->CreateBody({}));
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
        physicsObjectParameters_type["mass"]                            = &RigidBodyParameters::mass;
        physicsObjectParameters_type["shape"]                           = &RigidBodyParameters::shape;
        physicsObjectParameters_type["position"]                        = &RigidBodyParameters::position;
        physicsObjectParameters_type["scale"]                           = &RigidBodyParameters::scale;
        physicsObjectParameters_type["isStatic"]                        = &RigidBodyParameters::isStatic;
        physicsObjectParameters_type["customShapePositions"]            = &RigidBodyParameters::customShapePositions;

        sol::usertype<RigidBody3DProperties> physicsObjectParameters3D_type = state.new_usertype<RigidBody3DProperties>("RigidBodyParameters3D");
        physicsObjectParameters3D_type["mass"]                              = &RigidBody3DProperties::Mass;
        // physicsObjectParameters3D_type["shape"]                           = &RigidBody3DProperties::Shape;
        physicsObjectParameters3D_type["position"] = &RigidBody3DProperties::Position;
        // physicsObjectParameters3D_type["scale"]                           = &RigidBody3DProperties::Scale;
        physicsObjectParameters3D_type["isStatic"] = &RigidBody3DProperties::Static;
        // physicsObjectParameters3D_type["customShapePositions"]            = &RigidBody3DProperties::customShapePositions;

        sol::usertype<RigidBody3D> physics3D_type = state.new_usertype<RigidBody3D>("RigidBody3D"); //, sol::constructors<RigidBody2D>()); //;const RigidBodyParameters&)>());
        physics3D_type.set_function("SetForce", &RigidBody3D::SetForce);
        physics3D_type.set_function("SetPosition", &RigidBody3D::SetPosition);
        physics3D_type.set_function("SetLinearVelocity", &RigidBody3D::SetLinearVelocity);
        physics3D_type.set_function("GetLinearVelocity", &RigidBody3D::GetLinearVelocity);
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

        auto b2BodyDeftype = state.new_usertype<b2BodyDef>("b2BodyDef", sol::constructors<b2BodyDef()>());

        // Fields
        b2BodyDeftype["type"]            = &b2BodyDef::type;
        b2BodyDeftype["position"]        = &b2BodyDef::position;
        b2BodyDeftype["angle"]           = &b2BodyDef::angle;
        b2BodyDeftype["linearVelocity"]  = &b2BodyDef::linearVelocity;
        b2BodyDeftype["angularVelocity"] = &b2BodyDef::angularVelocity;
        b2BodyDeftype["linearDamping"]   = &b2BodyDef::linearDamping;
        b2BodyDeftype["angularDamping"]  = &b2BodyDef::angularDamping;
        b2BodyDeftype["allowSleep"]      = &b2BodyDef::allowSleep;
        b2BodyDeftype["awake"]           = &b2BodyDef::awake;
        b2BodyDeftype["fixedRotation"]   = &b2BodyDef::fixedRotation;
        b2BodyDeftype["bullet"]          = &b2BodyDef::bullet;
        b2BodyDeftype["userData"]        = &b2BodyDef::userData;
        b2BodyDeftype["gravityScale"]    = &b2BodyDef::gravityScale;

        auto b2Bodytype = state.new_usertype<b2Body>("b2Body");

        // Methods
        b2Bodytype["CreateFixture"] = sol::overload(
            sol::resolve<b2Fixture*(const b2FixtureDef*)>(&b2Body::CreateFixture),
            sol::resolve<b2Fixture*(const b2Shape*, float)>(&b2Body::CreateFixture));
        b2Bodytype["DestroyFixture"]                  = &b2Body::DestroyFixture;
        b2Bodytype["SetTransform"]                    = &b2Body::SetTransform;
        b2Bodytype["GetTransform"]                    = &b2Body::GetTransform;
        b2Bodytype["GetPosition"]                     = &b2Body::GetPosition;
        b2Bodytype["GetAngle"]                        = &b2Body::GetAngle;
        b2Bodytype["GetWorldCenter"]                  = &b2Body::GetWorldCenter;
        b2Bodytype["GetLocalCenter"]                  = &b2Body::GetLocalCenter;
        b2Bodytype["SetLinearVelocity"]               = &b2Body::SetLinearVelocity;
        b2Bodytype["GetLinearVelocity"]               = &b2Body::GetLinearVelocity;
        b2Bodytype["SetAngularVelocity"]              = &b2Body::SetAngularVelocity;
        b2Bodytype["GetAngularVelocity"]              = &b2Body::GetAngularVelocity;
        b2Bodytype["ApplyForce"]                      = &b2Body::ApplyForce;
        b2Bodytype["ApplyForceToCenter"]              = &b2Body::ApplyForceToCenter;
        b2Bodytype["ApplyTorque"]                     = &b2Body::ApplyTorque;
        b2Bodytype["ApplyLinearImpulse"]              = &b2Body::ApplyLinearImpulse;
        b2Bodytype["ApplyAngularImpulse"]             = &b2Body::ApplyAngularImpulse;
        b2Bodytype["GetMass"]                         = &b2Body::GetMass;
        b2Bodytype["GetInertia"]                      = &b2Body::GetInertia;
        b2Bodytype["GetMassData"]                     = &b2Body::GetMassData;
        b2Bodytype["SetMassData"]                     = &b2Body::SetMassData;
        b2Bodytype["ResetMassData"]                   = &b2Body::ResetMassData;
        b2Bodytype["GetWorldPoint"]                   = &b2Body::GetWorldPoint;
        b2Bodytype["GetWorldVector"]                  = &b2Body::GetWorldVector;
        b2Bodytype["GetLocalPoint"]                   = &b2Body::GetLocalPoint;
        b2Bodytype["GetLocalVector"]                  = &b2Body::GetLocalVector;
        b2Bodytype["GetLinearVelocityFromWorldPoint"] = &b2Body::GetLinearVelocityFromWorldPoint;
        b2Bodytype["GetLinearVelocityFromLocalPoint"] = &b2Body::GetLinearVelocityFromLocalPoint;
        b2Bodytype["GetLinearDamping"]                = &b2Body::GetLinearDamping;
        b2Bodytype["SetLinearDamping"]                = &b2Body::SetLinearDamping;
        b2Bodytype["GetAngularDamping"]               = &b2Body::GetAngularDamping;
        b2Bodytype["SetAngularDamping"]               = &b2Body::SetAngularDamping;
        b2Bodytype["GetGravityScale"]                 = &b2Body::GetGravityScale;
        b2Bodytype["SetGravityScale"]                 = &b2Body::SetGravityScale;
        b2Bodytype["SetType"]                         = &b2Body::SetType;
        b2Bodytype["GetType"]                         = &b2Body::GetType;
        b2Bodytype["SetBullet"]                       = &b2Body::SetBullet;
        b2Bodytype["IsBullet"]                        = &b2Body::IsBullet;
        b2Bodytype["SetSleepingAllowed"]              = &b2Body::SetSleepingAllowed;
        b2Bodytype["IsSleepingAllowed"]               = &b2Body::IsSleepingAllowed;
        b2Bodytype["SetAwake"]                        = &b2Body::SetAwake;
        b2Bodytype["IsAwake"]                         = &b2Body::IsAwake;
        b2Bodytype["SetFixedRotation"]                = &b2Body::SetFixedRotation;
        b2Bodytype["IsFixedRotation"]                 = &b2Body::IsFixedRotation;
        b2Bodytype["GetFixtureList"]                  = sol::overload(
            sol::resolve<b2Fixture*()>(&b2Body::GetFixtureList),
            sol::resolve<const b2Fixture*() const>(&b2Body::GetFixtureList));
        b2Bodytype["GetJointList"] = sol::overload(
            sol::resolve<b2JointEdge*()>(&b2Body::GetJointList),
            sol::resolve<const b2JointEdge*() const>(&b2Body::GetJointList));
        b2Bodytype["GetContactList"] = sol::overload(
            sol::resolve<b2ContactEdge*()>(&b2Body::GetContactList),
            sol::resolve<const b2ContactEdge*() const>(&b2Body::GetContactList));
        b2Bodytype["GetNext"] = sol::overload(
            sol::resolve<b2Body*()>(&b2Body::GetNext),
            sol::resolve<const b2Body*() const>(&b2Body::GetNext));
        b2Bodytype["GetUserData"] = &b2Body::GetUserData;
        b2Bodytype["GetWorld"]    = sol::overload(
            sol::resolve<b2World*()>(&b2Body::GetWorld),
            sol::resolve<const b2World*() const>(&b2Body::GetWorld));
        b2Bodytype["Dump"] = &b2Body::Dump;
    }
}
