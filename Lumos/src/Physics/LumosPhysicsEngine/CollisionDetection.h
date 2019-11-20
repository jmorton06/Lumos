#pragma once
#include "lmpch.h"
#include "PhysicsObject3D.h"
#include "CollisionShape.h"
#include "Manifold.h"
#include "Utilities/TSingleton.h"

#define CALL_MEMBER_FN(instance, ptrToMemberFn)  ((instance).*(ptrToMemberFn))

namespace Lumos
{
	struct LUMOS_EXPORT CollisionData
	{
		float penetration;
		Maths::Vector3 normal;
		Maths::Vector3 pointOnPlane;
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

		_FORCE_INLINE_ bool CheckCollision(const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, CollisionData* out_coldata = nullptr) const 
		{
			return CALL_MEMBER_FN(*this, m_CollisionCheckFunctions[shape1->GetType() | shape2->GetType()])(obj1, obj2, shape1, shape2, out_coldata);
		}

		bool BuildCollisionManifold(const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, const CollisionData& coldata, Manifold* out_manifold) const;


		static _FORCE_INLINE_ bool CheckSphereOverlap(const Maths::Vector3& pos1, float radius1, const Maths::Vector3& pos2, float radius2)
		{
			return (pos2 - pos1).LengthSquared() <= Maths::Squared(radius1 + radius2);
		}

		static _FORCE_INLINE_ bool CheckAABBOverlap(const Maths::Vector3& pos1, const Maths::Vector3& halfHidth1, const Maths::Vector3& pos2, const Maths::Vector3& halfHidth2)
		{
			if (abs(pos1.x - pos2.x) >= (halfHidth1.x + halfHidth2.x)) return false;
			if (abs(pos1.y - pos2.y) >= (halfHidth1.y + halfHidth2.y)) return false;
			if (abs(pos1.z - pos2.z) >= (halfHidth1.z + halfHidth2.z)) return false;
			return true;
		}

		static _FORCE_INLINE_ bool CheckAABBSphereOverlap(const Maths::Vector3& center, const Maths::Vector3& halfVol, const Maths::Vector3& spherePos, float sphereRad)
		{
			const Maths::Vector3 minVol = center - halfVol;
			const Maths::Vector3 maxVol = center + halfVol;
			float distSquared = sphereRad * sphereRad;

			if (spherePos.x <= minVol.x)			distSquared -= Maths::Squared(spherePos.x - minVol.x);
			else if (spherePos.x >= maxVol.x)	distSquared -= Maths::Squared(spherePos.x - maxVol.x);

			if (spherePos.y <= minVol.y)			distSquared -= Maths::Squared(spherePos.y - minVol.y);
			else if (spherePos.y >= maxVol.y)	distSquared -= Maths::Squared(spherePos.y - maxVol.y);

			if (spherePos.z <= minVol.z)			distSquared -= Maths::Squared(spherePos.z - minVol.z);
			else if (spherePos.z >= maxVol.z)	distSquared -= Maths::Squared(spherePos.z - maxVol.z);

			return distSquared > 0;
		}

		static _FORCE_INLINE_ bool CheckSphereInsideAABB(const Maths::Vector3& spherePos, float sphereRadius, const Maths::Vector3& AABBCenter, const Maths::Vector3& AABBHalfVol)
		{
			//min check
			Maths::Vector3 minPoint = AABBCenter - AABBHalfVol;
			if (minPoint.x > spherePos.x - sphereRadius) return false;
			if (minPoint.y > spherePos.y - sphereRadius) return false;
			if (minPoint.z > spherePos.z - sphereRadius) return false;
			//max check
			Maths::Vector3 maxPoint = AABBCenter + AABBHalfVol;
			if (maxPoint.x < spherePos.x + sphereRadius) return false;
			if (maxPoint.y < spherePos.y + sphereRadius) return false;
			if (maxPoint.z < spherePos.z + sphereRadius) return false;

			return true;
		}

		static _FORCE_INLINE_ bool CheckAABBInsideAABB(const Maths::Vector3& AABBInsideCenter, const Maths::Vector3& AABBInsideHalfVol, const Maths::Vector3& AABBCenter, const Maths::Vector3& AABBHalfVol)
		{
			//min check
			Maths::Vector3 minPoint = AABBCenter - AABBHalfVol;
			Maths::Vector3 minInsidePoint = AABBInsideCenter - AABBInsideHalfVol;
			if (minPoint.x > minInsidePoint.x) return false;
			if (minPoint.y > minInsidePoint.y) return false;
			if (minPoint.z > minInsidePoint.z) return false;
			//max check
			Maths::Vector3 maxPoint = AABBCenter + AABBHalfVol;
			Maths::Vector3 maxInsidePoint = AABBInsideCenter + AABBInsideHalfVol;
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
		static bool CheckCollisionAxis(const Maths::Vector3& axis, const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, CollisionData* out_coldata);

		static Maths::Vector3 GetClosestPointOnEdges(const Maths::Vector3& target, const std::vector<CollisionEdge>& edges);
		Maths::Vector3 PlaneEdgeIntersection(const Maths::Plane& plane, const Maths::Vector3& start, const Maths::Vector3& end) const;
		void	SutherlandHodgesonClipping(const std::list<Maths::Vector3>& input_polygon, int num_clip_planes, const Maths::Plane* clip_planes, std::list<Maths::Vector3>* out_polygon, bool removePoints) const;

	};
}
