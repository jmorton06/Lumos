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

    struct LuaCollisionCallback : public ContactCallback
    {
    public:
        void OnCollision(b2BodyId a, b2BodyId b, float approachSpeed) override
        {
            beginContactFunction(a, b, approachSpeed);
        }

        sol::function beginContactFunction;
    };

    static void SetCallback(const sol::function& func, b2BodyId bodyId)
    {
        sol::function beginContactFunction;
        LuaCollisionCallback* callback = new LuaCollisionCallback();
        callback->beginContactFunction = func;

        ContactCallback* oldCallback = (ContactCallback*)b2Body_GetUserData(bodyId);
        if(oldCallback)
            delete oldCallback;
        b2Body_SetUserData(bodyId, (void*)callback);
    }

    static void SetB2DGravity(const Vec2& gravity)
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
        physics2D_type.set_function("SetLinearDamping", &RigidBody2D::SetLinearDamping);

        physics2D_type.set_function("GetPosition", &RigidBody2D::GetPosition);
        physics2D_type.set_function("GetAngle", &RigidBody2D::GetAngle);
        physics2D_type.set_function("GetFriction", &RigidBody2D::GetFriction);
        physics2D_type.set_function("GetIsStatic", &RigidBody2D::GetIsStatic);
        physics2D_type.set_function("Init", &RigidBody2D::Init);
        physics2D_type.set_function("SetIsStatic", &RigidBody2D::SetIsStatic);
        physics2D_type.set_function("GetB2Body", &RigidBody2D::GetB2Body);

        state.set_function("SetCallback", &SetCallback);
        state.set_function("SetB2DGravity", &SetB2DGravity);
    }
}
