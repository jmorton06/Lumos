#pragma once
#include "LM.h"
#include "PhysicsObject3D.h"
#include "CollisionShape.h"
#include "Manifold.h"
#include "Utilities/TSingleton.h"
#include "Maths/MathsUtilities.h"

#define CALL_MEMBER_FN(instance, ptrToMemberFn)  ((instance).*(ptrToMemberFn))

namespace lumos
{
	struct LUMOS_EXPORT CollisionData
	{
		float penetration;
		maths::Vector3 normal;
		maths::Vector3 pointOnPlane;
	};

	class LUMOS_EXPORT CollisionDetection : public TSingleton<CollisionDetection>
	{
		friend class TSingleton<CollisionDetection>;
		typedef  bool (CollisionDetection::*CollisionCheckFunc)(const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, CollisionData* out_coldata) const;

		CollisionCheckFunc* m_CollisionCheckFunctions;

	public:
		CollisionDetection();
		~CollisionDetection()
		{
			if (m_CollisionCheckFunctions)	
				delete[] m_CollisionCheckFunctions;
		}

		inline bool CheckCollision(const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, CollisionData* out_coldata = nullptr) const 
		{
			return CALL_MEMBER_FN(*this, m_CollisionCheckFunctions[shape1->GetType() | shape2->GetType()])(obj1, obj2, shape1, shape2, out_coldata);
		}

		bool BuildCollisionManifold(const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, const CollisionData& coldata, Manifold* out_manifold) const;


		static inline bool CheckSphereOverlap(const maths::Vector3& pos1, float radius1, const maths::Vector3& pos2, float radius2)
		{
			return (pos2 - pos1).LengthSquared() <= maths::Squared(radius1 + radius2);
		}

		static inline bool CheckAABBOverlap(const maths::Vector3& pos1, const maths::Vector3& halfHidth1, const maths::Vector3& pos2, const maths::Vector3& halfHidth2)
		{
			if (abs(pos1.x - pos2.x) >= (halfHidth1.x + halfHidth2.x)) return false;
			if (abs(pos1.y - pos2.y) >= (halfHidth1.y + halfHidth2.y)) return false;
			if (abs(pos1.z - pos2.z) >= (halfHidth1.z + halfHidth2.z)) return false;
			return true;
		}

		static inline bool CheckAABBSphereOverlap(const maths::Vector3& center, const maths::Vector3& halfVol, const maths::Vector3& spherePos, float sphereRad)
		{
			const maths::Vector3 minVol = center - halfVol;
			const maths::Vector3 maxVol = center + halfVol;
			float distSquared = sphereRad * sphereRad;

			if (spherePos.x <= minVol.x)			distSquared -= maths::Squared(spherePos.x - minVol.x);
			else if (spherePos.x >= maxVol.x)	distSquared -= maths::Squared(spherePos.x - maxVol.x);

			if (spherePos.y <= minVol.y)			distSquared -= maths::Squared(spherePos.y - minVol.y);
			else if (spherePos.y >= maxVol.y)	distSquared -= maths::Squared(spherePos.y - maxVol.y);

			if (spherePos.z <= minVol.z)			distSquared -= maths::Squared(spherePos.z - minVol.z);
			else if (spherePos.z >= maxVol.z)	distSquared -= maths::Squared(spherePos.z - maxVol.z);

			return distSquared > 0;
		}

		static inline bool CheckSphereInsideAABB(const maths::Vector3& spherePos, float sphereRadius, const maths::Vector3& AABBCenter, const maths::Vector3& AABBHalfVol)
		{
			//min check
			maths::Vector3 minPoint = AABBCenter - AABBHalfVol;
			if (minPoint.x > spherePos.x - sphereRadius) return false;
			if (minPoint.y > spherePos.y - sphereRadius) return false;
			if (minPoint.z > spherePos.z - sphereRadius) return false;
			//max check
			maths::Vector3 maxPoint = AABBCenter + AABBHalfVol;
			if (maxPoint.x < spherePos.x + sphereRadius) return false;
			if (maxPoint.y < spherePos.y + sphereRadius) return false;
			if (maxPoint.z < spherePos.z + sphereRadius) return false;

			return true;
		}

		static inline bool CheckAABBInsideAABB(const maths::Vector3& AABBInsideCenter, const maths::Vector3& AABBInsideHalfVol, const maths::Vector3& AABBCenter, const maths::Vector3& AABBHalfVol)
		{
			//min check
			maths::Vector3 minPoint = AABBCenter - AABBHalfVol;
			maths::Vector3 minInsidePoint = AABBInsideCenter - AABBInsideHalfVol;
			if (minPoint.x > minInsidePoint.x) return false;
			if (minPoint.y > minInsidePoint.y) return false;
			if (minPoint.z > minInsidePoint.z) return false;
			//max check
			maths::Vector3 maxPoint = AABBCenter + AABBHalfVol;
			maths::Vector3 maxInsidePoint = AABBInsideCenter + AABBInsideHalfVol;
			if (maxPoint.x < maxInsidePoint.x) return false;
			if (maxPoint.y < maxInsidePoint.y) return false;
			if (maxPoint.z < maxInsidePoint.z) return false;

			return true;
		}

	protected:
		bool CheckPolyhedronCollision(const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, CollisionData* out_coldata = nullptr) const;
		bool CheckPolyhedronSphereCollision(const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, CollisionData* out_coldata = nullptr) const;
		bool CheckSphereCollision(const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, CollisionData* out_coldata = nullptr) const ;
		bool InvalidCheckCollision(const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, CollisionData* out_coldata = nullptr) const;
		static bool CheckCollisionAxis(const maths::Vector3& axis, const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, CollisionData* out_coldata);

		static maths::Vector3 GetClosestPointOnEdges(const maths::Vector3& target, const std::vector<CollisionEdge>& edges);
		maths::Vector3 PlaneEdgeIntersection(const maths::Plane& plane, const maths::Vector3& start, const maths::Vector3& end) const;
		void	SutherlandHodgesonClipping(const std::list<maths::Vector3>& input_polygon, int num_clip_planes, const maths::Plane* clip_planes, std::list<maths::Vector3>* out_polygon, bool removePoints) const;

	};
}
