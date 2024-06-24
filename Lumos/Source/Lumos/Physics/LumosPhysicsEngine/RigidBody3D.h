#pragma once
#include "Core/UUID.h"
#include "Core/DataStructures/TDArray.h"
#include "Maths/BoundingBox.h"
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Lumos
{
    class LumosPhysicsEngine;
    class Manifold;
    class CollisionShape;
    class RigidBody3D;

    enum CollisionShapeType : unsigned int;

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
        RigidBody3DProperties();
        ~RigidBody3DProperties();
        glm::vec3 Position        = glm::vec3(0.0f);
        glm::vec3 LinearVelocity  = glm::vec3(0.0f);
        glm::vec3 Force           = glm::vec3(0.0f);
        float Mass                = 1.0f;
        glm::quat Orientation     = glm::quat();
        glm::vec3 AngularVelocity = glm::vec3(0.0f);
        glm::vec3 Torque          = glm::vec3(0.0f);
        bool Static               = false;
        float Elasticity          = 0.5f;
        float Friction            = 0.5f;
        bool AtRest               = false;
        bool isTrigger            = false;
        SharedPtr<CollisionShape> Shape;
    };

    class alignas(16) RigidBody3D
    {
        friend class LumosPhysicsEngine;
        template <typename Archive>
        friend void save(Archive& archive, const RigidBody3D& rigidBody3D);

        template <typename Archive>
        friend void load(Archive& archive, RigidBody3D& rigidBody3D);

    public:
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
            return m_LocalBoundingBox;
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
            m_LocalBoundingBox  = bb;
            m_WSAabbInvalidated = true;
        }

        //<--------- SETTERS ------------->

        void SetPosition(const glm::vec3& v)
        {
            m_Position               = v;
            m_WSTransformInvalidated = true;
            m_WSAabbInvalidated      = true;
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
            m_WSTransformInvalidated = true;
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
            for(auto& callback : m_OnCollisionManifoldCallbacks)
                callback(a, b, manifold);
        }

        void AutoResizeBoundingBox();
        void RestTest();

        void DebugDraw(uint64_t flags) const;

        typedef std::function<void(RigidBody3D*, RigidBody3D*, Manifold*)> OnCollisionManifoldCallback;

        void AddOnCollisionManifoldCallback(const OnCollisionManifoldCallback callback) { m_OnCollisionManifoldCallbacks.PushBack(callback); }

        void SetCollisionShape(CollisionShapeType type);
        void SetCollisionShape(const SharedPtr<CollisionShape>& shape);
        void CollisionShapeUpdated();
        void SetInverseMass(const float& v);
        void SetMass(const float& v);
        const SharedPtr<CollisionShape>& GetCollisionShape() const;

        bool GetIsTrigger() const { return m_Trigger; }
        void SetIsTrigger(bool trigger) { m_Trigger = trigger; }

        float GetAngularFactor() const { return m_AngularFactor; }
        void SetAngularFactor(float factor) { m_AngularFactor = factor; }

        bool GetIsStatic() const { return m_Static; }
        bool GetIsAtRest() const { return m_AtRest; }
        float GetElasticity() const { return m_Elasticity; }
        float GetFriction() const { return m_Friction; }
        bool IsAwake() const { return !m_AtRest; }
        void SetElasticity(const float elasticity) { m_Elasticity = elasticity; }
        void SetFriction(const float friction) { m_Friction = friction; }
        void SetIsStatic(const bool isStatic)
        {
            m_Static = isStatic;
            if(m_Static)
                m_AtRest = true;
        }
        // void SetIsColliding(const bool colliding) { m_IsColliding = colliding; }
        UUID GetUUID() const { return m_UUID; }

        // For iteration
        RigidBody3D* m_Prev = nullptr;
        RigidBody3D* m_Next = nullptr;

        bool Valid() const { return m_UUID != 0; }

        uint16_t GetCollisionLayer() const { return m_CollisionLayer; }
        void SetCollisionLayer(u16 layer) { m_CollisionLayer = layer; }

    protected:
        RigidBody3D(const RigidBody3DProperties& properties = RigidBody3DProperties());

        float m_RestVelocityThresholdSquared;
        float m_AverageSummedVelocity;
        float m_Elasticity;
        float m_Friction;

        mutable glm::mat4 m_WSTransform;
        Maths::BoundingBox m_LocalBoundingBox; //!< Model orientated bounding box in model space

        mutable Maths::BoundingBox m_WSAabb; //!< Axis aligned bounding box of this object in world space

        mutable bool m_WSTransformInvalidated;
        mutable bool m_WSAabbInvalidated; //!< Flag indicating if the cached world space transoformed AABB is invalid
        bool m_Static;
        bool m_AtRest;
        float m_AngularFactor;

        UUID m_UUID;

        u16 m_CollisionLayer = 0;

        glm::vec3 m_Position;
        float m_InvMass;
        glm::vec3 m_LinearVelocity;
        bool m_Trigger = false;
        glm::vec3 m_Force;

        glm::quat m_Orientation;
        glm::vec3 m_AngularVelocity;
        glm::vec3 m_Torque;
        glm::mat3 m_InvInertia;

        SharedPtr<CollisionShape> m_CollisionShape;
        PhysicsCollisionCallback m_OnCollisionCallback;
        TDArray<OnCollisionManifoldCallback> m_OnCollisionManifoldCallbacks; //!< Collision callbacks post manifold generation
    };
}
