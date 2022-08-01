#pragma once

#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CollisionShape.h"
#include "Manifold.h"
#include "Utilities/TSingleton.h"

#define CALL_MEMBER_FN(instance, ptrToMemberFn) ((instance).*(ptrToMemberFn))

namespace Lumos
{
    struct LUMOS_EXPORT CollisionData
    {
        float penetration;
        glm::vec3 normal;
        glm::vec3 pointOnPlane;
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

        static inline bool CheckSphereOverlap(const glm::vec3& pos1, float radius1, const glm::vec3& pos2, float radius2)
        {
            return glm::distance2(pos2, pos1) <= Maths::Squared(radius1 + radius2);
        }

        static inline bool CheckAABBOverlap(const glm::vec3& pos1, const glm::vec3& halfHidth1, const glm::vec3& pos2, const glm::vec3& halfHidth2)
        {
            if(abs(pos1.x - pos2.x) >= (halfHidth1.x + halfHidth2.x))
                return false;
            if(abs(pos1.y - pos2.y) >= (halfHidth1.y + halfHidth2.y))
                return false;
            if(abs(pos1.z - pos2.z) >= (halfHidth1.z + halfHidth2.z))
                return false;
            return true;
        }

        static inline bool CheckAABBSphereOverlap(const glm::vec3& center, const glm::vec3& halfVol, const glm::vec3& spherePos, float sphereRad)
        {
            const glm::vec3 minVol = center - halfVol;
            const glm::vec3 maxVol = center + halfVol;
            float distSquared      = sphereRad * sphereRad;

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

        static inline bool CheckSphereInsideAABB(const glm::vec3& spherePos, float sphereRadius, const glm::vec3& AABBCenter, const glm::vec3& AABBHalfVol)
        {
            // min check
            glm::vec3 minPoint = AABBCenter - AABBHalfVol;
            if(minPoint.x > spherePos.x - sphereRadius)
                return false;
            if(minPoint.y > spherePos.y - sphereRadius)
                return false;
            if(minPoint.z > spherePos.z - sphereRadius)
                return false;
            // max check
            glm::vec3 maxPoint = AABBCenter + AABBHalfVol;
            if(maxPoint.x < spherePos.x + sphereRadius)
                return false;
            if(maxPoint.y < spherePos.y + sphereRadius)
                return false;
            if(maxPoint.z < spherePos.z + sphereRadius)
                return false;

            return true;
        }

        static inline bool CheckAABBInsideAABB(const glm::vec3& AABBInsideCenter, const glm::vec3& AABBInsideHalfVol, const glm::vec3& AABBCenter, const glm::vec3& AABBHalfVol)
        {
            // min check
            glm::vec3 minPoint       = AABBCenter - AABBHalfVol;
            glm::vec3 minInsidePoint = AABBInsideCenter - AABBInsideHalfVol;
            if(minPoint.x > minInsidePoint.x)
                return false;
            if(minPoint.y > minInsidePoint.y)
                return false;
            if(minPoint.z > minInsidePoint.z)
                return false;
            // max check
            glm::vec3 maxPoint       = AABBCenter + AABBHalfVol;
            glm::vec3 maxInsidePoint = AABBInsideCenter + AABBInsideHalfVol;
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
        bool CheckCapsuleCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);
        bool CheckCapsuleSphereCheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);
        bool CheckPolyhedronCapsuleCheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);
        bool InvalidCheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);

        static bool CheckCollisionAxis(const glm::vec3& axis, RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata);

        static glm::vec3 GetClosestPointOnEdges(const glm::vec3& target, const std::vector<CollisionEdge>& edges);
        glm::vec3 PlaneEdgeIntersection(const Plane& plane, const glm::vec3& start, const glm::vec3& end) const;
        void SutherlandHodgesonClipping(glm::vec3* input_polygon, int input_polygon_count, int num_clip_planes, const Plane* clip_planes, glm::vec3* output_polygon, int& output_polygon_count, bool removePoints) const;

        uint32_t m_MaxSize = 0;
    };
}
