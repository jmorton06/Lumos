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

        static bool CheckSphereOverlap(const glm::vec3& pos1, float radius1, const glm::vec3& pos2, float radius2);
        static bool CheckAABBOverlap(const glm::vec3& pos1, const glm::vec3& halfHidth1, const glm::vec3& pos2, const glm::vec3& halfHidth2);
        static bool CheckAABBSphereOverlap(const glm::vec3& center, const glm::vec3& halfVol, const glm::vec3& spherePos, float sphereRad);
        static bool CheckSphereInsideAABB(const glm::vec3& spherePos, float sphereRadius, const glm::vec3& AABBCenter, const glm::vec3& AABBHalfVol);
        static bool CheckAABBInsideAABB(const glm::vec3& AABBInsideCenter, const glm::vec3& AABBInsideHalfVol, const glm::vec3& AABBCenter, const glm::vec3& AABBHalfVol);

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
        void SutherlandHodgesonClipping(Arena* arena, const TDArray<glm::vec3>& input_polygon, int num_clip_planes, const Plane* clip_planes, TDArray<glm::vec3>* out_polygon, bool removePoints) const;
        uint32_t m_MaxSize = 0;
    };
}
