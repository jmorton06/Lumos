#include "Precompiled.h"
#include "PhysicsLua.h"
#include "Scene/Component/RigidBody2DComponent.h"
#include "Scene/Component/RigidBody3DComponent.h"
#include "Core/Application.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include <entt/entity/registry.hpp>
#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/TerrainCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Physics/LumosPhysicsEngine/PhysicsMaterial.h"
#include "Physics/LumosPhysicsEngine/RaycastResult.h"
#include "Physics/LumosPhysicsEngine/RaycastVehicle.h"
#include "Core/DataStructures/TDArray.h"
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

    static void SetGravity3D(const Vec3& gravity)
    {
        Application::Get().GetSystem<LumosPhysicsEngine>()->SetGravity(gravity);
    }

    static Vec3 GetGravity3D()
    {
        return Application::Get().GetSystem<LumosPhysicsEngine>()->GetGravity();
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
        physicsObjectParameters_type["friction"]                        = &RigidBodyParameters::friction;
        physicsObjectParameters_type["damping"]                         = &RigidBodyParameters::damping;
        physicsObjectParameters_type["elasticity"]                      = &RigidBodyParameters::elasticity;

        sol::usertype<CollisionInfo3D> collisionInfo3D_type = state.new_usertype<CollisionInfo3D>("CollisionInfo3D");
        collisionInfo3D_type["otherBody"]      = &CollisionInfo3D::OtherBody;
        collisionInfo3D_type["contactNormal"]   = &CollisionInfo3D::ContactNormal;
        collisionInfo3D_type["contactPoint"]    = &CollisionInfo3D::ContactPoint;
        collisionInfo3D_type["penetration"]     = &CollisionInfo3D::Penetration;
        collisionInfo3D_type["isTrigger"]       = &CollisionInfo3D::IsTrigger;

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
        physics3D_type.set_function("SetIsStatic", &RigidBody3D::SetIsStatic);
        physics3D_type.set_function("SetCollisionShape", static_cast<void (RigidBody3D::*)(CollisionShapeType)>(&RigidBody3D::SetCollisionShape));
        physics3D_type.set_function("GetLinearFactor", &RigidBody3D::GetLinearFactor);
        physics3D_type.set_function("SetLinearFactor", &RigidBody3D::SetLinearFactor);
        physics3D_type.set_function("GetAngularFactor", &RigidBody3D::GetAngularFactor);
        physics3D_type.set_function("SetAngularFactor", &RigidBody3D::SetAngularFactor);
        physics3D_type.set_function("GetMaterial", &RigidBody3D::GetMaterial);
        physics3D_type.set_function("SetMaterial", &RigidBody3D::SetMaterial);
        physics3D_type.set_function("GetElasticity", &RigidBody3D::GetElasticity);
        physics3D_type.set_function("SetElasticity", &RigidBody3D::SetElasticity);
        physics3D_type.set_function("GetIsTrigger", &RigidBody3D::GetIsTrigger);
        physics3D_type.set_function("SetIsTrigger", &RigidBody3D::SetIsTrigger);

        // Additional getters/setters
        physics3D_type.set_function("GetForce", &RigidBody3D::GetForce);
        physics3D_type.set_function("GetTorque", &RigidBody3D::GetTorque);
        physics3D_type.set_function("SetTorque", &RigidBody3D::SetTorque);
        physics3D_type.set_function("GetAngularVelocity", &RigidBody3D::GetAngularVelocity);
        physics3D_type.set_function("GetOrientation", &RigidBody3D::GetOrientation);
        physics3D_type.set_function("GetInverseMass", &RigidBody3D::GetInverseMass);
        physics3D_type.set_function("GetMass", [](const RigidBody3D& b) -> float
            { float inv = b.GetInverseMass(); return inv > 0.0f ? 1.0f / inv : 0.0f; });
        physics3D_type.set_function("SetMass", &RigidBody3D::SetMass);
        physics3D_type.set_function("WakeUp", &RigidBody3D::WakeUp);
        physics3D_type.set_function("IsAwake", &RigidBody3D::IsAwake);
        physics3D_type.set_function("GetCollisionLayer", &RigidBody3D::GetCollisionLayer);
        physics3D_type.set_function("SetCollisionLayer", &RigidBody3D::SetCollisionLayer);
        physics3D_type.set_function("GetCollisionMask", &RigidBody3D::GetCollisionMask);
        physics3D_type.set_function("SetCollisionMask", &RigidBody3D::SetCollisionMask);

        // Additive force/impulse helpers (engine only has Set; gameplay usually wants Apply)
        physics3D_type.set_function("AddForce", [](RigidBody3D& b, const Vec3& f)
            { b.SetForce(b.GetForce() + f); });
        physics3D_type.set_function("AddTorque", [](RigidBody3D& b, const Vec3& t)
            { b.SetTorque(b.GetTorque() + t); });
        physics3D_type.set_function("ApplyImpulse", [](RigidBody3D& b, const Vec3& impulse)
            { b.SetLinearVelocity(b.GetLinearVelocity() + impulse * b.GetInverseMass()); b.WakeUp(); });
        physics3D_type.set_function("ApplyAngularImpulse", [](RigidBody3D& b, const Vec3& impulse)
            { b.SetAngularVelocity(b.GetAngularVelocity() + b.GetInverseInertia() * impulse); b.WakeUp(); });

        // Off-centre force/impulse + point velocity (needed for vehicles, ropes, etc.)
        physics3D_type.set_function("ApplyImpulseAtPoint", &RigidBody3D::ApplyImpulseAtPoint);
        physics3D_type.set_function("ApplyForceAtPoint", &RigidBody3D::ApplyForceAtPoint);
        physics3D_type.set_function("GetPointVelocity", &RigidBody3D::GetPointVelocity);

        // Raycast bindings (LumosPhysicsEngine 3D)
        sol::usertype<RaycastHit> raycastHit_type = state.new_usertype<RaycastHit>("RaycastHit");
        raycastHit_type["body"]     = &RaycastHit::Body;
        raycastHit_type["point"]    = &RaycastHit::Point;
        raycastHit_type["normal"]   = &RaycastHit::Normal;
        raycastHit_type["distance"] = &RaycastHit::Distance;
        raycastHit_type.set_function("Hit", &RaycastHit::Hit);

        sol::usertype<RaycastQuery> raycastQuery_type = state.new_usertype<RaycastQuery>("RaycastQuery",
            sol::constructors<RaycastQuery(), RaycastQuery(const Vec3&, const Vec3&, float)>());
        raycastQuery_type["origin"]      = &RaycastQuery::Origin;
        raycastQuery_type["direction"]   = &RaycastQuery::Direction;
        raycastQuery_type["maxDistance"] = &RaycastQuery::MaxDistance;
        raycastQuery_type["layerMask"]   = &RaycastQuery::LayerMask;
        raycastQuery_type["hitTriggers"] = &RaycastQuery::HitTriggers;

        state.set_function("Raycast", [](const Vec3& origin, const Vec3& dir, float maxDist) -> RaycastHit
            { return Application::Get().GetSystem<LumosPhysicsEngine>()->Raycast(origin, dir, maxDist); });

        // Direct heightmap sample (world-space x, z). Walks the rigid body
        // list looking for the first TerrainCollisionShape, samples it. Skips
        // the raycast path entirely so it works regardless of the raycast
        // implementation's terrain support.
        state.set_function("TerrainHeightAt", [](float wx, float wz) -> sol::optional<float>
            {
                auto* phys = Application::Get().GetSystem<LumosPhysicsEngine>();
                if(!phys) return sol::nullopt;
                auto* scene = Application::Get().GetCurrentScene();
                if(!scene) return sol::nullopt;
                auto& reg = scene->GetRegistry();
                auto view = reg.view<RigidBody3DComponent>();
                for(auto e : view)
                {
                    auto* body = view.get<RigidBody3DComponent>(e).GetRigidBody();
                    if(!body) continue;
                    auto shape = body->GetCollisionShape();
                    if(!shape || shape->GetType() != CollisionShapeType::CollisionTerrain)
                        continue;
                    auto* terrain = static_cast<TerrainCollisionShape*>(shape.get());
                    if(!terrain->HasHeightData()) continue;
                    Vec3 bp = body->GetPosition();
                    float h = terrain->GetHeightAt(wx - bp.x, wz - bp.z);
                    return bp.y + h;
                }
                return sol::nullopt;
            });
        state.set_function("RaycastAll", [](const Vec3& origin, const Vec3& dir, float maxDist) -> sol::as_table_t<std::vector<RaycastHit>>
            {
                TDArray<RaycastHit> hits;
                Application::Get().GetSystem<LumosPhysicsEngine>()->RaycastAll(RaycastQuery(origin, dir, maxDist), hits);
                std::vector<RaycastHit> out;
                out.reserve(hits.Size());
                for(uint32_t i = 0; i < hits.Size(); ++i) out.push_back(hits[i]);
                return sol::as_table(out);
            });

        sol::usertype<PhysicsMaterial> physicsMaterial_type = state.new_usertype<PhysicsMaterial>("PhysicsMaterial");
        physicsMaterial_type["friction"]         = &PhysicsMaterial::Friction;
        physicsMaterial_type["restitution"]      = &PhysicsMaterial::Restitution;
        physicsMaterial_type["rolling_friction"] = &PhysicsMaterial::RollingFriction;
        physicsMaterial_type.set_function("Default", &PhysicsMaterial::Default);
        physicsMaterial_type.set_function("Bouncy", &PhysicsMaterial::Bouncy);
        physicsMaterial_type.set_function("Ice", &PhysicsMaterial::Ice);
        physicsMaterial_type.set_function("Rubber", &PhysicsMaterial::Rubber);
        physicsMaterial_type.set_function("Metal", &PhysicsMaterial::Metal);
        physicsMaterial_type.set_function("Wood", &PhysicsMaterial::Wood);
        physicsMaterial_type.set_function("Concrete", &PhysicsMaterial::Concrete);

        std::initializer_list<std::pair<sol::string_view, Shape>> shapes = {
            { "Square", Shape::Square },
            { "Circle", Shape::Circle },
            { "Custom", Shape::Custom }
        };
        state.new_enum<Shape, false>("Shape", shapes);

        std::initializer_list<std::pair<sol::string_view, CollisionShapeType>> shapes3D = {
            { "Cuboid", CollisionShapeType::CollisionCuboid },
            { "Sphere", CollisionShapeType::CollisionSphere },
            { "Pyramid", CollisionShapeType::CollisionPyramid },
            { "Capsule", CollisionShapeType::CollisionCapsule },
            { "Hull", CollisionShapeType::CollisionHull },
            { "Terrain", CollisionShapeType::CollisionTerrain }
        };
        state.new_enum<CollisionShapeType, false>("CollisionShapeType", shapes3D);

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

        // ---- Raycast vehicle ----
        sol::usertype<VehicleWheelConfig> wheelConfig_type = state.new_usertype<VehicleWheelConfig>("VehicleWheelConfig");
        wheelConfig_type["connectionPoint"]    = &VehicleWheelConfig::ConnectionPointLocal;
        wheelConfig_type["radius"]             = &VehicleWheelConfig::Radius;
        wheelConfig_type["suspensionRest"]     = &VehicleWheelConfig::SuspensionRest;
        wheelConfig_type["suspensionStiffness"] = &VehicleWheelConfig::SuspensionStiffness;
        wheelConfig_type["suspensionDamping"]  = &VehicleWheelConfig::SuspensionDamping;
        wheelConfig_type["maxTravel"]          = &VehicleWheelConfig::MaxTravel;
        wheelConfig_type["gripLateral"]        = &VehicleWheelConfig::GripLateral;
        wheelConfig_type["gripForward"]        = &VehicleWheelConfig::GripForward;
        wheelConfig_type["steerable"]          = &VehicleWheelConfig::Steerable;
        wheelConfig_type["powered"]            = &VehicleWheelConfig::Powered;
        wheelConfig_type["brakes"]             = &VehicleWheelConfig::Brakes;

        sol::usertype<RaycastVehicle> vehicle_type = state.new_usertype<RaycastVehicle>("RaycastVehicle");
        vehicle_type.set_function("AddWheel", &RaycastVehicle::AddWheel);
        vehicle_type.set_function("SetInputs", [](RaycastVehicle& v, float throttle, float brake, float steer, sol::optional<bool> handbrake)
            { v.SetInputs(throttle, brake, steer, handbrake.value_or(false)); });
        vehicle_type.set_function("NumWheels", &RaycastVehicle::NumWheels);
        vehicle_type.set_function("IsGrounded", &RaycastVehicle::IsGrounded);
        vehicle_type.set_function("WheelPosition", &RaycastVehicle::WheelPosition);
        vehicle_type.set_function("WheelRotation", &RaycastVehicle::WheelRotation);
        vehicle_type.set_function("GetForwardSpeed", &RaycastVehicle::GetForwardSpeed);
        vehicle_type.set_function("SetEngineForce", &RaycastVehicle::SetEngineForce);
        vehicle_type.set_function("SetBrakeForce", &RaycastVehicle::SetBrakeForce);
        vehicle_type.set_function("SetMaxSteerAngle", &RaycastVehicle::SetMaxSteerAngle);
        vehicle_type.set_function("SetSteerSpeed", &RaycastVehicle::SetSteerSpeed);
        vehicle_type.set_function("SetAntiRoll", &RaycastVehicle::SetAntiRoll);
        vehicle_type.set_function("SetGrip", &RaycastVehicle::SetGrip);
        vehicle_type.set_function("SetSuspension", &RaycastVehicle::SetSuspension);

        auto physics = state["Physics"].get_or_create<sol::table>();
        physics.set_function("SetGravity3D", &SetGravity3D);
        physics.set_function("GetGravity3D", &GetGravity3D);
        physics.set_function("CreateVehicle", [](RigidBody3D* chassis) -> RaycastVehicle*
            { return Application::Get().GetSystem<LumosPhysicsEngine>()->CreateVehicle(chassis); });
        physics.set_function("DestroyVehicle", [](RaycastVehicle* v)
            { Application::Get().GetSystem<LumosPhysicsEngine>()->DestroyVehicle(v); });

        // Build properties for a sized cuboid body (Lua can't set the shape on
        // RigidBodyParameters3D directly). Pass to entity:AddRigidBody3DComponent.
        physics.set_function("BoxProperties", [](const Vec3& pos, const Vec3& halfExtents, float mass, bool isStatic) -> RigidBody3DProperties
            {
                RigidBody3DProperties p;
                p.Position                    = pos;
                p.Mass                        = mass > 0.0f ? mass : 1.0f;
                p.Static                      = isStatic;
                SharedPtr<CollisionShape> box = CreateSharedPtr<CuboidCollisionShape>(halfExtents);
                p.Shape                       = box;
                return p;
            });
    }
}
