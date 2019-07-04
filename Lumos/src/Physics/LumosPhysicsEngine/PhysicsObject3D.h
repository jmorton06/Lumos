#pragma once

#include "LM.h"
#include "Physics/PhysicsObject.h"
#include "Maths/Quaternion.h"
#include "Maths/Matrix3.h"
#include "CollisionShape.h"
#include "Maths/BoundingBox.h"
#include <memory>

namespace Lumos
{
	class LumosPhysicsEngine;
	class Entity;
	class Manifold;

	//Callback function called whenever a collision is detected between two objects
	//Params:
	//	PhysicsObject3D* this_obj			- The current object class that contains the callback
	//	PhysicsObject3D* colliding_obj	- The object that is colliding with the given object
	//Return:
	//  True	- The physics engine should process the collision as normal
	//	False	- The physics engine should drop the collision pair and not do any further collision resolution/manifold generation
	//			  > This can be useful for AI to see if a player/agent is inside an area/collision volume
	typedef std::function<bool(PhysicsObject3D* this_obj, PhysicsObject3D* colliding_obj)> PhysicsCollisionCallback;

	class LUMOS_EXPORT PhysicsObject3D : public PhysicsObject
	{
		friend class LumosPhysicsEngine;

	public:
		PhysicsObject3D();
		virtual ~PhysicsObject3D();

		//<--------- GETTERS ------------->
		const Maths::Vector3&	 GetPosition()			  const { return m_Position; }
		const Maths::Vector3&	 GetLinearVelocity()	  const { return m_LinearVelocity; }
		const Maths::Vector3&	 GetForce()				  const { return m_Force; }
		float					 GetInverseMass()		  const { return m_InvMass; }
		const Maths::Quaternion& GetOrientation()	      const { return m_Orientation; }
		const Maths::Vector3&	 GetAngularVelocity()	  const { return m_AngularVelocity; }
		const Maths::Vector3&	 GetTorque()			  const { return m_Torque; }
		const Maths::Matrix3&	 GetInverseInertia()	  const { return m_InvInertia; }
		CollisionShape*			 GetCollisionShape()	  const { return m_CollisionShape.get(); }
		Entity*					 GetAssociatedObject()	  const { return m_pParent; }
		const Maths::Matrix4&	 GetWorldSpaceTransform() const;	//Built from scratch or returned from cached value

		Maths::BoundingBox GetWorldSpaceAABB() const;

		void WakeUp() override;
		void SetIsAtRest(const bool isAtRest) override;

		Maths::BoundingBox GetLocalBoundingBox() const
		{
			return m_localBoundingBox;
		}

		void SetRestVelocityThreshold(float vel)
		{
			if (vel <= 0.0f)
				m_RestVelocityThresholdSquared = -1.0f;
			else
				m_RestVelocityThresholdSquared = vel * vel;
		}

		void SetLocalBoundingBox(const Maths::BoundingBox &bb)
		{
			m_localBoundingBox = bb;
			m_wsAabbInvalidated = true;
		}

		//<--------- SETTERS ------------->

		void SetPosition(const Maths::Vector3& v)
		{
			m_Position = v;
			m_wsTransformInvalidated = true;
			m_wsAabbInvalidated = true;
			//m_AtRest = false;
		}

        void SetLinearVelocity(const Maths::Vector3& v) { if(m_Static) return; m_LinearVelocity = v; }
		void SetForce(const Maths::Vector3& v) { if(m_Static) return; m_Force = v; }
		void SetInverseMass(const float& v) { m_InvMass = v; }

		void SetOrientation(const Maths::Quaternion& v)
		{
			m_Orientation = v;
			m_wsTransformInvalidated = true;
			//m_AtRest = false;
		}

		void SetAngularVelocity(const Maths::Vector3& v) { if(m_Static) return; m_AngularVelocity = v; }
		void SetTorque(const Maths::Vector3& v) {if(m_Static) return;  m_Torque = v; }
		void SetInverseInertia(const Maths::Matrix3& v) { m_InvInertia = v; }

		void SetCollisionShape(std::unique_ptr<CollisionShape> colShape) { m_CollisionShape = std::move(colShape); AutoResizeBoundingBox(); }

		//Called automatically when PhysicsObject3D is created through Object3D::CreatePhysicsNode()
		void SetAssociatedObject(Entity* obj) { m_pParent = obj; }

		//<---------- CALLBACKS ------------>
		void SetOnCollisionCallback(PhysicsCollisionCallback& callback) { m_OnCollisionCallback = callback; }

		bool FireOnCollisionEvent(PhysicsObject3D *obj_a, PhysicsObject3D *obj_b)
		{
			const bool handleCollision = (m_OnCollisionCallback) ? m_OnCollisionCallback(obj_a, obj_b) : true;

			// Wake up on collision
			if (handleCollision)
				WakeUp();

			return handleCollision;
		}

		void FireOnCollisionManifoldCallback(PhysicsObject3D* a, PhysicsObject3D* b, Manifold *manifold)
		{
			for (auto it = m_onCollisionManifoldCallbacks.begin(); it != m_onCollisionManifoldCallbacks.end(); ++it)
				it->operator()(a, b, manifold);
		}

		void AutoResizeBoundingBox();
		void RestTest();

		virtual void DebugDraw(uint64_t flags) const;

		typedef std::function<void(PhysicsObject3D*, PhysicsObject3D*, Manifold *)> OnCollisionManifoldCallback;

		void AddOnCollisionManifoldCallback(const OnCollisionManifoldCallback callback)
		{
			m_onCollisionManifoldCallbacks.push_back(callback);
		}

		void SetEntity(Entity* entity){ m_pParent = entity; }

	protected:
		Entity*				m_pParent;			//Optional: Attached GameObject or NULL if none set
		mutable bool		m_wsTransformInvalidated;
		float				m_RestVelocityThresholdSquared;
		float				m_AverageSummedVelocity;

		mutable Maths::Matrix4 	   m_wsTransform;
		Maths::BoundingBox		   m_localBoundingBox;   //!< Model orientated bounding box in model space
		mutable bool			   m_wsAabbInvalidated;  //!< Flag indicating if the cached world space transoformed AABB is invalid
		mutable Maths::BoundingBox m_wsAabb;			 //!< Axis aligned bounding box of this object in world space

		//<---------LINEAR-------------->
		Maths::Vector3		m_Position;
		Maths::Vector3		m_LinearVelocity;
		Maths::Vector3		m_Force;
		float		m_InvMass;

		//<----------ANGULAR-------------->
		Maths::Quaternion  m_Orientation;
		Maths::Vector3	   m_AngularVelocity;
		Maths::Vector3	   m_Torque;
		Maths::Matrix3	   m_InvInertia;

		//<----------COLLISION------------>
		std::unique_ptr<CollisionShape> m_CollisionShape;
		PhysicsCollisionCallback	    m_OnCollisionCallback;
		std::vector<OnCollisionManifoldCallback> m_onCollisionManifoldCallbacks; //!< Collision callbacks post manifold generation

	};
}
