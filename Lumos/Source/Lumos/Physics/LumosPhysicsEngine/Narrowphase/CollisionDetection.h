#pragma once

#include "RigidBody3D.h"
#include "CollisionShape.h"
#include "Manifold.h"
#include "Utilities/TSingleton.h"

#define CALL_MEMBER_FN(instance, ptrToMemberFn) ((instance).*(ptrToMemberFn))

namespace Lumos
{
    struct LUMOS_EXPORT CollisionData
    {
        float penetration;
        Maths::Vector3 normal;
        Maths::Vector3 pointOnPlane;
    };

    class LUMOS_EXPORT CollisionDetection : public ThreadSafeSingleton<CollisionDetection>
    {
        friend class TSingleton<CollisionDetection>;
        typedef bool (CollisionDetection::*CollisionCheckFunc)(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata);

        CollisionCheckFunc* m_CollisionCheckFunctions;

    public:
        CollisionDetection();
        ~CollisionDetection()
        {
            if(m_CollisionCheckFunctions)
                delete[] m_CollisionCheckFunctions;
        }

        bool CheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);

        bool BuildCollisionManifold(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData& coldata, Manifold* out_manifold);

        static inline bool CheckSphereOverlap(const Maths::Vector3& pos1, float radius1, const Maths::Vector3& pos2, float radius2)
        {
            return (pos2 - pos1).LengthSquared() <= Maths::Squared(radius1 + radius2);
        }

        static inline bool CheckAABBOverlap(const Maths::Vector3& pos1, const Maths::Vector3& halfHidth1, const Maths::Vector3& pos2, const Maths::Vector3& halfHidth2)
        {
            if(abs(pos1.x - pos2.x) >= (halfHidth1.x + halfHidth2.x))
                return false;
            if(abs(pos1.y - pos2.y) >= (halfHidth1.y + halfHidth2.y))
                return false;
            if(abs(pos1.z - pos2.z) >= (halfHidth1.z + halfHidth2.z))
                return false;
            return true;
        }

        static inline bool CheckAABBSphereOverlap(const Maths::Vector3& center, const Maths::Vector3& halfVol, const Maths::Vector3& spherePos, float sphereRad)
        {
            const Maths::Vector3 minVol = center - halfVol;
            const Maths::Vector3 maxVol = center + halfVol;
            float distSquared = sphereRad * sphereRad;

            if(spherePos.x <= minVol.x)
                distSquared -= Maths::Squared(spherePos.x - minVol.x);
            else if(spherePos.x >= maxVol.x)
                distSquared -= Maths::Squared(spherePos.x - maxVol.x);

            if(spherePos.y <= minVol.y)
                distSquared -= Maths::Squared(spherePos.y - minVol.y);
            else if(spherePos.y >= maxVol.y)
                distSquared -= Maths::Squared(spherePos.y - maxVol.y);

            if(spherePos.z <= minVol.z)
                distSquared -= Maths::Squared(spherePos.z - minVol.z);
            else if(spherePos.z >= maxVol.z)
                distSquared -= Maths::Squared(spherePos.z - maxVol.z);

            return distSquared > 0;
        }

        static inline bool CheckSphereInsideAABB(const Maths::Vector3& spherePos, float sphereRadius, const Maths::Vector3& AABBCenter, const Maths::Vector3& AABBHalfVol)
        {
            //min check
            Maths::Vector3 minPoint = AABBCenter - AABBHalfVol;
            if(minPoint.x > spherePos.x - sphereRadius)
                return false;
            if(minPoint.y > spherePos.y - sphereRadius)
                return false;
            if(minPoint.z > spherePos.z - sphereRadius)
                return false;
            //max check
            Maths::Vector3 maxPoint = AABBCenter + AABBHalfVol;
            if(maxPoint.x < spherePos.x + sphereRadius)
                return false;
            if(maxPoint.y < spherePos.y + sphereRadius)
                return false;
            if(maxPoint.z < spherePos.z + sphereRadius)
                return false;

            return true;
        }

        static inline bool CheckAABBInsideAABB(const Maths::Vector3& AABBInsideCenter, const Maths::Vector3& AABBInsideHalfVol, const Maths::Vector3& AABBCenter, const Maths::Vector3& AABBHalfVol)
        {
            //min check
            Maths::Vector3 minPoint = AABBCenter - AABBHalfVol;
            Maths::Vector3 minInsidePoint = AABBInsideCenter - AABBInsideHalfVol;
            if(minPoint.x > minInsidePoint.x)
                return false;
            if(minPoint.y > minInsidePoint.y)
                return false;
            if(minPoint.z > minInsidePoint.z)
                return false;
            //max check
            Maths::Vector3 maxPoint = AABBCenter + AABBHalfVol;
            Maths::Vector3 maxInsidePoint = AABBInsideCenter + AABBInsideHalfVol;
            if(maxPoint.x < maxInsidePoint.x)
                return false;
            if(maxPoint.y < maxInsidePoint.y)
                return false;
            if(maxPoint.z < maxInsidePoint.z)
                return false;

            return true;
        }

    protected:
        bool CheckPolyhedronCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);
        bool CheckPolyhedronSphereCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);
        bool CheckSphereCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);
        bool InvalidCheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);
        static bool CheckCollisionAxis(const Maths::Vector3& axis, RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata);

        static Maths::Vector3 GetClosestPointOnEdges(const Maths::Vector3& target, const std::vector<CollisionEdge>& edges);
        Maths::Vector3 PlaneEdgeIntersection(const Maths::Plane& plane, const Maths::Vector3& start, const Maths::Vector3& end) const;
        void SutherlandHodgesonClipping(Maths::Vector3* input_polygon, int input_polygon_count, int num_clip_planes, const Maths::Plane* clip_planes, Maths::Vector3* output_polygon, int& output_polygon_count, bool removePoints) const;

        uint32_t m_MaxSize = 0;
    };
}
