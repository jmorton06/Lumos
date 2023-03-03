#pragma once
#include "Physics/LumosPhysicsEngine/CollisionShapes/CollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/HullCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CapsuleCollisionShape.h"
#include "Maths/Maths.h"
#include <cereal/types/polymorphic.hpp>
#include <cereal/cereal.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "Core/UUID.h"

CEREAL_REGISTER_TYPE(Lumos::SphereCollisionShape);
CEREAL_REGISTER_TYPE(Lumos::CuboidCollisionShape);
CEREAL_REGISTER_TYPE(Lumos::PyramidCollisionShape);
CEREAL_REGISTER_TYPE(Lumos::HullCollisionShape);
CEREAL_REGISTER_TYPE(Lumos::CapsuleCollisionShape);

CEREAL_REGISTER_POLYMORPHIC_RELATION(Lumos::CollisionShape, Lumos::SphereCollisionShape);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Lumos::CollisionShape, Lumos::CuboidCollisionShape);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Lumos::CollisionShape, Lumos::PyramidCollisionShape);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Lumos::CollisionShape, Lumos::HullCollisionShape);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Lumos::CollisionShape, Lumos::CapsuleCollisionShape);

namespace Lumos
{
    class LumosPhysicsEngine;
    class Manifold;

    // Callback function called whenever a collision is detected between two objects
    // Params:
    //	RigidBody3D* this_obj			- The current object class that contains the callback
    //	RigidBody3D* colliding_obj	- The object that is colliding with the given object
    // Return:
    //   True	- The physics engine should process the collision as normal
    //	False	- The physics engine should drop the collision pair and not do any further collision resolution/manifold generation
    //			  > This can be useful for AI to see if a player/agent is inside an area/collision volume
    typedef std::function<bool(RigidBody3D* this_obj, RigidBody3D* colliding_obj)> PhysicsCollisionCallback;

    struct RigidBody3DProperties
    {
        glm::vec3 Position              = glm::vec3(0.0f);
        glm::vec3 LinearVelocity        = glm::vec3(0.0f);
        glm::vec3 Force                 = glm::vec3(0.0f);
        float Mass                      = 1.0f;
        glm::quat Orientation           = glm::quat();
        glm::vec3 AngularVelocity       = glm::vec3(0.0f);
        glm::vec3 Torque                = glm::vec3(0.0f);
        bool Static                     = false;
        float Elasticity                = 1.0f;
        float Friction                  = 1.0f;
        bool AtRest                     = false;
        bool isTrigger                  = false;
        SharedPtr<CollisionShape> Shape = nullptr;
    };

    class LUMOS_EXPORT RigidBody3D
    {
        friend class LumosPhysicsEngine;

    public:
        RigidBody3D(const RigidBody3DProperties& properties = RigidBody3DProperties());
        ~RigidBody3D();

        //<--------- GETTERS ------------->
        const glm::vec3& GetPosition() const { return m_Position; }
        const glm::vec3& GetLinearVelocity() const { return m_LinearVelocity; }
        const glm::vec3& GetForce() const { return m_Force; }
        float GetInverseMass() const { return m_InvMass; }
        const glm::quat& GetOrientation() const { return m_Orientation; }
        const glm::vec3& GetAngularVelocity() const { return m_AngularVelocity; }
        const glm::vec3& GetTorque() const { return m_Torque; }
        const glm::mat3& GetInverseInertia() const { return m_InvInertia; }
        const glm::mat4& GetWorldSpaceTransform() const; // Built from scratch or returned from cached value

        const Maths::BoundingBox& GetWorldSpaceAABB();

        void WakeUp();
        void SetIsAtRest(const bool isAtRest);

        Maths::BoundingBox GetLocalBoundingBox() const
        {
            return m_localBoundingBox;
        }

        void SetRestVelocityThreshold(float vel)
        {
            if(vel <= 0.0f)
                m_RestVelocityThresholdSquared = -1.0f;
            else
                m_RestVelocityThresholdSquared = vel * vel;
        }

        void SetLocalBoundingBox(const Maths::BoundingBox& bb)
        {
            m_localBoundingBox  = bb;
            m_wsAabbInvalidated = true;
        }

        //<--------- SETTERS ------------->

        void SetPosition(const glm::vec3& v)
        {
            m_Position               = v;
            m_wsTransformInvalidated = true;
            m_wsAabbInvalidated      = true;
            // m_AtRest = false;
        }

        void SetLinearVelocity(const glm::vec3& v)
        {
            if(m_Static)
                return;
            m_LinearVelocity = v;
            m_AtRest         = false;
        }
        void SetForce(const glm::vec3& v)
        {
            if(m_Static)
                return;
            m_Force  = v;
            m_AtRest = false;
        }

        void SetOrientation(const glm::quat& v)
        {
            m_Orientation            = v;
            m_wsTransformInvalidated = true;
            m_AtRest                 = false;
        }

        void SetAngularVelocity(const glm::vec3& v)
        {
            if(m_Static)
                return;
            m_AngularVelocity = v;

            if(glm::length(v) > 0.0f)
                m_AtRest = false;
        }
        void SetTorque(const glm::vec3& v)
        {
            if(m_Static)
                return;
            m_Torque = v;
            m_AtRest = false;
        }
        void SetInverseInertia(const glm::mat3& v) { m_InvInertia = v; }

        //<---------- CALLBACKS ------------>
        void SetOnCollisionCallback(PhysicsCollisionCallback& callback) { m_OnCollisionCallback = callback; }
        bool FireOnCollisionEvent(RigidBody3D* obj_a, RigidBody3D* obj_b)
        {
            const bool handleCollision = (m_OnCollisionCallback) ? m_OnCollisionCallback(obj_a, obj_b) : true;

            // Wake up on collision
            if(handleCollision)
                WakeUp();

            return handleCollision;
        }

        void FireOnCollisionManifoldCallback(RigidBody3D* a, RigidBody3D* b, Manifold* manifold)
        {
            for(auto it = m_onCollisionManifoldCallbacks.begin(); it != m_onCollisionManifoldCallbacks.end(); ++it)
                it->operator()(a, b, manifold);
        }

        void AutoResizeBoundingBox();
        void RestTest();

        void DebugDraw(uint64_t flags) const;

        typedef std::function<void(RigidBody3D*, RigidBody3D*, Manifold*)> OnCollisionManifoldCallback;

        void AddOnCollisionManifoldCallback(const OnCollisionManifoldCallback callback) { m_onCollisionManifoldCallbacks.push_back(callback); }

        void SetCollisionShape(const SharedPtr<CollisionShape>& shape)
        {
            m_CollisionShape = shape;
            m_InvInertia     = m_CollisionShape->BuildInverseInertia(m_InvMass);
            AutoResizeBoundingBox();
        }

        void SetCollisionShape(CollisionShapeType type);

        void CollisionShapeUpdated()
        {
            if(m_CollisionShape)
                m_InvInertia = m_CollisionShape->BuildInverseInertia(m_InvMass);
            AutoResizeBoundingBox();
        }

        void SetInverseMass(const float& v)
        {
            m_InvMass = v;
            if(m_CollisionShape)
                m_InvInertia = m_CollisionShape->BuildInverseInertia(m_InvMass);
        }

        void SetMass(const float& v)
        {
            LUMOS_ASSERT(v > 0, "Physics object mass <= 0");
            m_InvMass = 1.0f / v;

            if(m_CollisionShape)
                m_InvInertia = m_CollisionShape->BuildInverseInertia(m_InvMass);
        }

        const SharedPtr<CollisionShape>& GetCollisionShape() const
        {
            return m_CollisionShape;
        }

        bool GetIsTrigger() const { return m_Trigger; }
        void SetIsTrigger(bool trigger) { m_Trigger = trigger; }

        float GetAngularFactor() const { return m_AngularFactor; }
        void SetAngularFactor(float factor) { m_AngularFactor = factor; }

        template <typename Archive>
        void save(Archive& archive) const
        {
            auto shape = std::unique_ptr<CollisionShape>(m_CollisionShape.get());

            const int Version = 1;

            archive(cereal::make_nvp("Version", Version));

            archive(cereal::make_nvp("Position", m_Position), cereal::make_nvp("Orientation", m_Orientation), cereal::make_nvp("LinearVelocity", m_LinearVelocity), cereal::make_nvp("Force", m_Force), cereal::make_nvp("Mass", 1.0f / m_InvMass), cereal::make_nvp("AngularVelocity", m_AngularVelocity), cereal::make_nvp("Torque", m_Torque), cereal::make_nvp("Static", m_Static), cereal::make_nvp("Friction", m_Friction), cereal::make_nvp("Elasticity", m_Elasticity), cereal::make_nvp("CollisionShape", shape), cereal::make_nvp("Trigger", m_Trigger), cereal::make_nvp("AngularFactor", m_AngularFactor));

            shape.release();
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            auto shape = std::unique_ptr<CollisionShape>(m_CollisionShape.get());

            int Version;
            archive(cereal::make_nvp("Version", Version));

            archive(cereal::make_nvp("Position", m_Position), cereal::make_nvp("Orientation", m_Orientation), cereal::make_nvp("LinearVelocity", m_LinearVelocity), cereal::make_nvp("Force", m_Force), cereal::make_nvp("Mass", 1.0f / m_InvMass), cereal::make_nvp("AngularVelocity", m_AngularVelocity), cereal::make_nvp("Torque", m_Torque), cereal::make_nvp("Static", m_Static), cereal::make_nvp("Friction", m_Friction), cereal::make_nvp("Elasticity", m_Elasticity), cereal::make_nvp("CollisionShape", shape), cereal::make_nvp("Trigger", m_Trigger), cereal::make_nvp("AngularFactor", m_AngularFactor));

            m_CollisionShape = SharedPtr<CollisionShape>(shape.get());
            CollisionShapeUpdated();
            shape.release();
        }

        bool GetIsStatic() const { return m_Static; }
        bool GetIsAtRest() const { return m_AtRest; }
        float GetElasticity() const { return m_Elasticity; }
        float GetFriction() const { return m_Friction; }
        bool IsAwake() const { return !m_AtRest; }
        void SetElasticity(const float elasticity) { m_Elasticity = elasticity; }
        void SetFriction(const float friction) { m_Friction = friction; }
        void SetIsStatic(const bool isStatic) { m_Static = isStatic; }

        UUID GetUUID() const { return m_UUID; }

    protected:
        mutable bool m_wsTransformInvalidated;
        float m_RestVelocityThresholdSquared;
        float m_AverageSummedVelocity;

        mutable glm::mat4 m_wsTransform;
        Maths::BoundingBox m_localBoundingBox; //!< Model orientated bounding box in model space
        mutable bool m_wsAabbInvalidated;      //!< Flag indicating if the cached world space transoformed AABB is invalid
        mutable Maths::BoundingBox m_wsAabb;   //!< Axis aligned bounding box of this object in world space

        bool m_Static;
        float m_Elasticity;
        float m_Friction;
        bool m_AtRest;
        UUID m_UUID;

        //<---------LINEAR-------------->
        glm::vec3 m_Position;
        glm::vec3 m_LinearVelocity;
        glm::vec3 m_Force;
        float m_InvMass;
        bool m_Trigger = false;

        //<----------ANGULAR-------------->
        glm::quat m_Orientation;
        glm::vec3 m_AngularVelocity;
        glm::vec3 m_Torque;
        glm::mat3 m_InvInertia;
        float m_AngularFactor;

        //<----------COLLISION------------>
        SharedPtr<CollisionShape> m_CollisionShape;
        PhysicsCollisionCallback m_OnCollisionCallback;
        std::vector<OnCollisionManifoldCallback> m_onCollisionManifoldCallbacks; //!< Collision callbacks post manifold generation
    };
}
