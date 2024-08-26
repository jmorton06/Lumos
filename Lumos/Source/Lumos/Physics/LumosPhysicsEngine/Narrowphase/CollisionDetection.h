#pragma once

#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CollisionShape.h"
#include "Manifold.h"
#include "Utilities/TSingleton.h"
#include "Core/DataStructures/TDArray.h"

#define CALL_MEMBER_FN(instance, ptrToMemberFn) ((instance).*(ptrToMemberFn))

namespace Lumos
{
    struct LUMOS_EXPORT CollisionData
    {
        float penetration;
        Vec3 normal;
        Vec3 pointOnPlane;
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

        static bool CheckSphereOverlap(const Vec3& pos1, float radius1, const Vec3& pos2, float radius2);
        static bool CheckAABBOverlap(const Vec3& pos1, const Vec3& halfHidth1, const Vec3& pos2, const Vec3& halfHidth2);
        static bool CheckAABBSphereOverlap(const Vec3& center, const Vec3& halfVol, const Vec3& spherePos, float sphereRad);
        static bool CheckSphereInsideAABB(const Vec3& spherePos, float sphereRadius, const Vec3& AABBCenter, const Vec3& AABBHalfVol);
        static bool CheckAABBInsideAABB(const Vec3& AABBInsideCenter, const Vec3& AABBInsideHalfVol, const Vec3& AABBCenter, const Vec3& AABBHalfVol);

    protected:
        bool CheckPolyhedronCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);
        bool CheckPolyhedronSphereCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);
        bool CheckSphereCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);
        bool CheckCapsuleCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);
        bool CheckCapsuleSphereCheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);
        bool CheckPolyhedronCapsuleCheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);
        bool InvalidCheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata = nullptr);

        static bool CheckCollisionAxis(const Vec3& axis, RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata);

        static Vec3 GetClosestPointOnEdges(const Vec3& target, const TDArray<CollisionEdge>& edges);
        Vec3 PlaneEdgeIntersection(const Plane& plane, const Vec3& start, const Vec3& end) const;
        void SutherlandHodgesonClipping(Arena* arena, const TDArray<Vec3>& input_polygon, int num_clip_planes, const Plane* clip_planes, TDArray<Vec3>* out_polygon, bool removePoints) const;
        uint32_t m_MaxSize = 0;
    };
}
