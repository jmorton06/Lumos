#include "Precompiled.h"
#include "CollisionDetection.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/SphereCollisionShape.h"
#include "Maths/MathsUtilities.h"
#include <glm/gtx/string_cast.hpp>

namespace Lumos
{
    CollisionDetection::CollisionDetection()
    {
        m_MaxSize                 = CollisionShapeTypeMax | (CollisionShapeTypeMax >> 1);
        m_CollisionCheckFunctions = new CollisionCheckFunc[m_MaxSize];
        std::fill(m_CollisionCheckFunctions, m_CollisionCheckFunctions + m_MaxSize, &CollisionDetection::CheckPolyhedronCollision);

        m_CollisionCheckFunctions[CollisionSphere]  = &CollisionDetection::CheckSphereCollision;
        m_CollisionCheckFunctions[CollisionCuboid]  = &CollisionDetection::CheckPolyhedronCollision;
        m_CollisionCheckFunctions[CollisionPyramid] = &CollisionDetection::CheckPolyhedronCollision;
        m_CollisionCheckFunctions[CollisionHull]    = &CollisionDetection::CheckPolyhedronCollision;
        m_CollisionCheckFunctions[CollisionCapsule] = &CollisionDetection::CheckCapsuleCollision;

        m_CollisionCheckFunctions[CollisionSphere | CollisionCuboid]  = &CollisionDetection::CheckPolyhedronSphereCollision;
        m_CollisionCheckFunctions[CollisionSphere | CollisionPyramid] = &CollisionDetection::CheckPolyhedronSphereCollision;
        m_CollisionCheckFunctions[CollisionSphere | CollisionHull]    = &CollisionDetection::CheckPolyhedronSphereCollision;

        m_CollisionCheckFunctions[CollisionSphere | CollisionCapsule]  = &CollisionDetection::CheckCapsuleSphereCheckCollision;
        m_CollisionCheckFunctions[CollisionCapsule | CollisionCuboid]  = &CollisionDetection::CheckPolyhedronCapsuleCheckCollision;
        m_CollisionCheckFunctions[CollisionCapsule | CollisionPyramid] = &CollisionDetection::CheckPolyhedronCapsuleCheckCollision;
        m_CollisionCheckFunctions[CollisionCapsule | CollisionHull]    = &CollisionDetection::CheckPolyhedronCapsuleCheckCollision;
    }

    bool CollisionDetection::CheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_ASSERT(((shape1->GetType() | shape2->GetType()) < m_MaxSize), "Invalid collision func {0}, {1}, {2}, {3}", shape1->GetType(), shape2->GetType(), shape2->GetType() | shape2->GetType(), m_MaxSize);
        return CALL_MEMBER_FN(*this, m_CollisionCheckFunctions[shape1->GetType() | shape2->GetType()])(obj1, obj2, shape1, shape2, out_coldata);
    }

    bool CollisionDetection::InvalidCheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_LOG_CRITICAL("Invalid Collision type specified");
        return false;
    }

    bool CollisionDetection::CheckSphereCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_ASSERT(shape1->GetType() == CollisionShapeType::CollisionSphere && shape2->GetType() == CollisionShapeType::CollisionSphere, "Both shapes are not spheres");

        //        CollisionData colData;
        //        glm::vec3 axis = obj2->GetPosition() - obj1->GetPosition();
        //
        //        float sumRadii        = ((SphereCollisionShape*)shape1)->GetRadius() + ((SphereCollisionShape*)shape2)->GetRadius();
        //        float sumRadiiSquared = sumRadii * sumRadii;
        //        float distSquared     = glm::length2(axis);
        //        if(distSquared > sumRadiiSquared)
        //            return false;
        //
        //        colData.normal       = glm::normalize(axis);
        //        colData.penetration  = sumRadii - std::sqrt(distSquared);
        //        colData.pointOnPlane = obj1->GetPosition() + axis * 0.5f;
        //
        //        if(out_coldata)
        //            *out_coldata = colData;

        CollisionData colData;
        glm::vec3 axis = obj2->GetPosition() - obj1->GetPosition();
        axis           = glm::normalize(axis);
        if(!CheckCollisionAxis(axis, obj1, obj2, shape1, shape2, &colData))
            return false;

        if(out_coldata)
            *out_coldata = colData;

        return true;
    }

    void AddPossibleCollisionAxis(glm::vec3& axis, glm::vec3* possibleCollisionAxes, uint32_t& possibleCollisionAxesCount)
    {
        LUMOS_PROFILE_FUNCTION();
        if(glm::length2(axis) < Maths::M_EPSILON)
            return;

        axis = glm::normalize(axis);

        float value = (1.0f - Maths::M_EPSILON);

        for(uint32_t i = 0; i < possibleCollisionAxesCount; i++)
        {
            const glm::vec3& p_axis = possibleCollisionAxes[i];
            if(glm::abs(glm::dot(axis, p_axis)) >= value)
                return;
        }

        possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
    }

    bool CollisionDetection::CheckPolyhedronSphereCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_ASSERT(shape1->GetType() == CollisionShapeType::CollisionSphere || shape2->GetType() == CollisionShapeType::CollisionSphere, "No sphere collision shape");

        CollisionShape* complexShape;
        RigidBody3D* complexObj;
        RigidBody3D* sphereObj;

        if(obj1->GetCollisionShape()->GetType() == CollisionShapeType::CollisionSphere)
        {
            sphereObj    = obj1;
            complexShape = shape2;
            complexObj   = obj2;
        }
        else
        {
            sphereObj    = obj2;
            complexShape = shape1;
            complexObj   = obj1;
        }

        CollisionData cur_colData;
        CollisionData best_colData;
        best_colData.penetration = -FLT_MAX;

        std::vector<glm::vec3>& shapeCollisionAxes      = complexShape->GetCollisionAxes(complexObj);
        std::vector<CollisionEdge>& complex_shape_edges = complexShape->GetEdges(complexObj);

        glm::vec3 p   = GetClosestPointOnEdges(sphereObj->GetPosition(), complex_shape_edges);
        glm::vec3 p_t = sphereObj->GetPosition() - p;
        p_t           = glm::normalize(p_t);

        static const int MAX_COLLISION_AXES = 100;
        static glm::vec3 possibleCollisionAxes[MAX_COLLISION_AXES];

        uint32_t possibleCollisionAxesCount = 0;
        for(const glm::vec3& axis : shapeCollisionAxes)
        {
            possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
        }

        AddPossibleCollisionAxis(p_t, possibleCollisionAxes, possibleCollisionAxesCount);

        for(uint32_t i = 0; i < possibleCollisionAxesCount; i++)
        {
            const glm::vec3& axis = possibleCollisionAxes[i];
            if(!CheckCollisionAxis(axis, obj1, obj2, shape1, shape2, &cur_colData))
                return false;

            if(cur_colData.penetration > best_colData.penetration)
                best_colData = cur_colData;
        }

        if(out_coldata)
            *out_coldata = best_colData;

        return true;
    }

    bool CollisionDetection::CheckPolyhedronCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION();
        CollisionData cur_colData;
        CollisionData best_colData;
        best_colData.penetration = -FLT_MAX;

        std::vector<glm::vec3>& shape1CollisionAxes         = shape1->GetCollisionAxes(obj1);
        std::vector<glm::vec3>& shape2PossibleCollisionAxes = shape2->GetCollisionAxes(obj2);

        static const int MAX_COLLISION_AXES = 100;
        static glm::vec3 possibleCollisionAxes[MAX_COLLISION_AXES];

        uint32_t possibleCollisionAxesCount = 0;
        for(const glm::vec3& axis : shape1CollisionAxes)
        {
            possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
        }

        for(const glm::vec3& axis : shape2PossibleCollisionAxes)
        {
            possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
        }

        std::vector<CollisionEdge>& shape1_edges = shape1->GetEdges(obj1);
        std::vector<CollisionEdge>& shape2_edges = shape2->GetEdges(obj2);

        for(const CollisionEdge& edge1 : shape1_edges)
        {
            for(const CollisionEdge& edge2 : shape2_edges)
            {
                glm::vec3 e1 = edge1.posB - edge1.posA;
                glm::vec3 e2 = edge2.posB - edge2.posA;
                e1           = glm::normalize(e1);
                e2           = glm::normalize(e2);

                glm::vec3 temp = glm::cross(e1, e2);
                AddPossibleCollisionAxis(temp, possibleCollisionAxes, possibleCollisionAxesCount);
            }
        }

        for(uint32_t i = 0; i < possibleCollisionAxesCount; i++)
        {
            const glm::vec3& axis = possibleCollisionAxes[i];
            if(!CheckCollisionAxis(axis, obj1, obj2, shape1, shape2, &cur_colData))
                return false;

            if(cur_colData.penetration >= best_colData.penetration)
                best_colData = cur_colData;
        }

        if(out_coldata)
            *out_coldata = best_colData;

        return true;
    }

    float PlaneSegmentIntersection(const glm::vec3& segA, const glm::vec3& segB, const float planeD, const glm::vec3& planeNormal)
    {
        float t = -1.0f;

        const float nDotAB = glm::dot(planeNormal, segB - segA);

        // If the segment is not parallel to the plane
        if(std::abs(nDotAB) > Maths::M_EPSILON)
        {
            t = (planeD - glm::dot(planeNormal, segA)) / nDotAB;
        }

        return t;
    }

    float PointToLineDistance(const glm::vec3& linePointA, const glm::vec3& linePointB, const glm::vec3& point)
    {
        float distAB = glm::length(linePointB - linePointA);

        if(distAB < Maths::M_EPSILON)
        {
            return glm::length(point - linePointA);
        }

        return (glm::length(glm::cross((point - linePointA), (point - linePointB)))) / distAB;
    }

    bool CollisionDetection::CheckCapsuleCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        CapsuleCollisionShape* capsuleShape1 = static_cast<CapsuleCollisionShape*>(shape1);
        CapsuleCollisionShape* capsuleShape2 = static_cast<CapsuleCollisionShape*>(shape2);

        CollisionData cur_colData;
        CollisionData best_colData;
        best_colData.penetration = -FLT_MAX;

        float capsule1Radius = capsuleShape1->GetRadius();
        float capsule2Radius = capsuleShape2->GetRadius();

        glm::vec3 p1 = obj1->GetPosition();
        glm::vec3 p2 = obj2->GetPosition();

        glm::vec3 d = p2 - p1;
        float dist  = glm::length(d);

        float height1 = capsuleShape1->GetHeight();
        float height2 = capsuleShape2->GetHeight();

        float capsule1HalfHeight = height1 * 0.5f;
        float capsule2HalfHeight = height2 * 0.5f;

        float radiusSum = capsule1Radius + capsule2Radius;

        glm::mat4 capsule1ToCapsule2SpaceTransform = glm::inverse(obj2->GetWorldSpaceTransform()) * obj1->GetWorldSpaceTransform();

        glm::vec3 capsule1SegA(0, -capsule1HalfHeight, 0);
        glm::vec3 capsule1SegB(0, capsule1HalfHeight, 0);
        capsule1SegA = capsule1ToCapsule2SpaceTransform * capsule1SegA;
        capsule1SegB = capsule1ToCapsule2SpaceTransform * capsule1SegB;

        glm::vec3 capsule2SegA(0, -capsule2HalfHeight, 0);
        glm::vec3 capsule2SegB(0, capsule2HalfHeight, 0);

        // The two inner capsule segments
        const glm::vec3 seg1 = capsule1SegB - capsule1SegA;
        const glm::vec3 seg2 = capsule2SegB - capsule2SegA;
        bool areParallel     = Maths::AreVectorsParallel(seg1, seg2);

        if(areParallel)
        {
            float segmentsPerpendicularDistance = PointToLineDistance(capsule1SegA, capsule1SegB, capsule2SegA);
            if(segmentsPerpendicularDistance > radiusSum)
                return false;

            float d1 = glm::dot(seg1, glm::vec3(capsule1SegA));
            float d2 = -glm::dot(seg1, glm::vec3(capsule1SegB));

            float t1 = PlaneSegmentIntersection(capsule2SegB, capsule2SegA, d1, seg1);
            float t2 = PlaneSegmentIntersection(capsule2SegA, capsule2SegB, d2, -seg1);

            if(t1 > 0.0f && t2 > 0.0f)
            {
                // Clip the inner segment of capsule 2
                if(t1 > 1.0f)
                    t1 = 1.0f;
                const glm::vec3 clipPointA = capsule2SegB - t1 * seg2;
                if(t2 > 1.0f)
                    t2 = 1.0f;
                const glm::vec3 clipPointB = capsule2SegA + t2 * seg2;

                // Project point capsule2SegA onto line of inner segment of capsule 1
                const glm::vec3 seg1Normalized    = glm::normalize(seg1);
                glm::vec3 pointOnInnerSegCapsule1 = glm::vec3(capsule1SegA) + glm::dot(seg1Normalized, glm::vec3(capsule2SegA - capsule1SegA)) * seg1Normalized;

                glm::vec3 normalCapsule2SpaceNormalized;
                glm::vec3 segment1ToSegment2;

                // If the inner capsule segments perpendicular distance is not zero (the inner segments are not overlapping)
                if(segmentsPerpendicularDistance > Maths::M_EPSILON)
                {
                    // Compute a perpendicular vector from segment 1 to segment 2
                    segment1ToSegment2            = (capsule2SegA - pointOnInnerSegCapsule1);
                    normalCapsule2SpaceNormalized = glm::normalize(segment1ToSegment2);
                }
                else
                {
                    glm::vec3 vec1(1, 0, 0);
                    glm::vec3 vec2(0, 1, 0);

                    glm::vec3 seg2Normalized = glm::normalize(seg2);

                    float cosA1        = std::abs(seg2Normalized.x);
                    float cosA2        = std::abs(seg2Normalized.y);
                    segment1ToSegment2 = glm::vec3(0.0f, 0.0f, 0.0f);

                    normalCapsule2SpaceNormalized = cosA1 < cosA2 ? glm::cross(seg2Normalized, vec1) : glm::cross(seg2Normalized, vec2);
                }

                glm::mat4 capsule2ToCapsule1SpaceTransform = glm::inverse(capsule1ToCapsule2SpaceTransform);
                const glm::vec3 contactPointACapsule1Local = capsule2ToCapsule1SpaceTransform * glm::vec4(clipPointA - segment1ToSegment2 + normalCapsule2SpaceNormalized * capsule1Radius, 1.0f);
                const glm::vec3 contactPointBCapsule1Local = capsule2ToCapsule1SpaceTransform * glm::vec4(clipPointB - segment1ToSegment2 + normalCapsule2SpaceNormalized * capsule1Radius, 1.0f);
                const glm::vec3 contactPointACapsule2Local = clipPointA - normalCapsule2SpaceNormalized * capsule2Radius;
                const glm::vec3 contactPointBCapsule2Local = clipPointB - normalCapsule2SpaceNormalized * capsule2Radius;

                float penetrationDepth = radiusSum - segmentsPerpendicularDistance;

                glm::vec3 normalWorld = glm::mat4(obj2->GetOrientation()) * glm::vec4(normalCapsule2SpaceNormalized, 1.0f);
                // glm::vec3 normalWorld = obj2->GetOrientation() * normalCapsule2SpaceNormalized;

                float correlation1 = glm::dot(normalWorld, glm::vec3(contactPointACapsule1Local));
                float correlation2 = glm::dot(normalWorld, glm::vec3(contactPointBCapsule1Local));

                static bool flipNormal = false;
                // flipNormal = !flipNormal;
                // if(correlation1 <= correlation2)
                //  if(glm::dot(normalWorld, p2 - p1) < 0.0f)
                //  {
                //      normalWorld = -normalWorld;
                //  }

                // Flip Normal if needed
                if(glm::dot(normalWorld, p2 - p1) < 0.0f)
                // if(flipNormal)
                {
                    normalWorld = -normalWorld;
                }

                best_colData.normal       = glm::normalize(normalWorld);
                best_colData.penetration  = penetrationDepth;
                best_colData.pointOnPlane = obj1->GetWorldSpaceTransform() * glm::vec4(contactPointACapsule1Local, 1.0f);

                if(out_coldata)
                    *out_coldata = best_colData;

                return true;
            }
        }

        glm::vec3 closestPointCapsule1Seg;
        glm::vec3 closestPointCapsule2Seg;
        Maths::ClosestPointBetweenTwoSegments(capsule1SegA, capsule1SegB, capsule2SegA, capsule2SegB,
                                              closestPointCapsule1Seg, closestPointCapsule2Seg);

        glm::vec3 closestPointsSeg1ToSeg2       = (closestPointCapsule2Seg - closestPointCapsule1Seg);
        const float closestPointsDistanceSquare = glm::length2(closestPointsSeg1ToSeg2);

        if(closestPointsDistanceSquare < radiusSum * radiusSum)
        {
            if(closestPointsDistanceSquare > Maths::M_EPSILON)
            {
                float closestPointsDistance = std::sqrt(closestPointsDistanceSquare);
                closestPointsSeg1ToSeg2 /= closestPointsDistance;

                const glm::vec3 contactPointCapsule1Local = glm::inverse(capsule1ToCapsule2SpaceTransform) * (closestPointCapsule1Seg + closestPointsSeg1ToSeg2 * capsule1Radius);
                const glm::vec3 contactPointCapsule2Local = closestPointCapsule2Seg - closestPointsSeg1ToSeg2 * capsule2Radius;

                // const glm::vec3 normalWorld = obj2->GetOrientation() * closestPointsSeg1ToSeg2;
                glm::vec3 normalWorld = glm::mat4(obj2->GetOrientation()) * glm::vec4(closestPointsSeg1ToSeg2, 1.0f);

                float penetrationDepth = radiusSum - closestPointsDistance;

                // Create the contact info object

                float correlation1 = glm::dot(normalWorld, glm::vec3(contactPointCapsule1Local));
                float correlation2 = glm::dot(normalWorld, glm::vec3(contactPointCapsule2Local));

                // if(correlation1 <= correlation2)
                if(glm::dot(normalWorld, p2 - p1) < 0.0f)
                {
                    normalWorld = -normalWorld;
                }

                best_colData.normal       = glm::normalize(normalWorld);
                best_colData.penetration  = penetrationDepth;
                best_colData.pointOnPlane = obj1->GetWorldSpaceTransform() * glm::vec4(contactPointCapsule1Local, 1.0f);

                if(out_coldata)
                    *out_coldata = best_colData;

                return true;
            }
            else
            {
                if(areParallel)
                {
                    float squareDistCapsule2PointToCapsuleSegA = glm::length2((capsule1SegA - closestPointCapsule2Seg));

                    glm::vec3 capsule1SegmentMostExtremePoint = squareDistCapsule2PointToCapsuleSegA > Maths::M_EPSILON ? capsule1SegA : capsule1SegB;
                    glm::vec3 normalCapsuleSpace2             = (closestPointCapsule2Seg - capsule1SegmentMostExtremePoint);
                    normalCapsuleSpace2 = glm::normalize(normalCapsuleSpace2);

                    const glm::vec3 contactPointCapsule1Local = glm::inverse(capsule1ToCapsule2SpaceTransform) * (closestPointCapsule1Seg + normalCapsuleSpace2 * capsule1Radius);
                    const glm::vec3 contactPointCapsule2Local = closestPointCapsule2Seg - normalCapsuleSpace2 * capsule2Radius;

                    // const glm::vec3 normalWorld = obj2->GetOrientation() * glm::vec4(normalCapsuleSpace2, 1.0f);
                    glm::vec3 normalWorld = glm::mat4(obj2->GetOrientation()) * glm::vec4(normalCapsuleSpace2, 1.0f);

                    float correlation1 = glm::dot(normalWorld, glm::vec3(contactPointCapsule1Local));
                    float correlation2 = glm::dot(normalWorld, glm::vec3(contactPointCapsule2Local));

                    // if(correlation1 <= correlation2)
                    if(glm::dot(normalWorld, p2 - p1) < 0.0f)
                    {
                        normalWorld = -normalWorld;
                    }
                    // Create the contact info object
                    best_colData.normal       = glm::normalize(normalWorld);
                    best_colData.penetration  = radiusSum;
                    best_colData.pointOnPlane = obj1->GetWorldSpaceTransform() * glm::vec4(contactPointCapsule1Local, 1.0f);

                    if(out_coldata)
                        *out_coldata = best_colData;

                    return true;
                }
                else
                {
                    glm::vec3 normalCapsuleSpace2 = glm::cross(seg1, seg2);
                    glm::normalize(normalCapsuleSpace2);

                    const glm::vec3 contactPointCapsule1Local = glm::inverse(capsule1ToCapsule2SpaceTransform) * (closestPointCapsule1Seg + normalCapsuleSpace2 * capsule1Radius);
                    const glm::vec3 contactPointCapsule2Local = closestPointCapsule2Seg - normalCapsuleSpace2 * capsule2Radius;

                    // const glm::vec3 normalWorld = obj2->GetOrientation() * glm::vec4(normalCapsuleSpace2, 1.0f);
                    glm::vec3 normalWorld = glm::mat4(obj2->GetOrientation()) * glm::vec4(normalCapsuleSpace2, 1.0f);

                    float correlation1 = glm::dot(normalWorld, glm::vec3(contactPointCapsule1Local));
                    float correlation2 = glm::dot(normalWorld, glm::vec3(contactPointCapsule2Local));

                    // if(correlation1 <= correlation2)
                    if(glm::dot(normalWorld, p2 - p1) < 0.0f)
                    {
                        normalWorld = -normalWorld;
                    }

                    best_colData.normal       = glm::normalize(normalWorld);
                    best_colData.penetration  = radiusSum;
                    best_colData.pointOnPlane = obj1->GetWorldSpaceTransform() * glm::vec4(contactPointCapsule1Local, 1.0f);

                    if(out_coldata)
                        *out_coldata = best_colData;

                    return true;
                }
            }
        }

        return false;
    }

    bool CollisionDetection::CheckCapsuleSphereCheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_ASSERT(shape1->GetType() == CollisionShapeType::CollisionSphere || shape2->GetType() == CollisionShapeType::CollisionSphere, "Both shapes are not spheres");

        CollisionData colData;

        CapsuleCollisionShape* capsuleShape;
        SphereCollisionShape* sphereShape;
        RigidBody3D* capsuleObj;
        RigidBody3D* sphereObj;

        if(obj1->GetCollisionShape()->GetType() == CollisionShapeType::CollisionSphere)
        {
            sphereObj    = obj1;
            sphereShape  = (SphereCollisionShape*)shape1;
            capsuleShape = (CapsuleCollisionShape*)shape2;
            capsuleObj   = obj2;
        }
        else
        {
            capsuleObj   = obj1;
            sphereObj    = obj2;
            sphereShape  = (SphereCollisionShape*)shape2;
            capsuleShape = (CapsuleCollisionShape*)shape1;
        }

        float sphereRadius      = sphereShape->GetRadius();
        float capsuleHeight     = capsuleShape->GetHeight();
        float capsuleRadius     = capsuleShape->GetRadius();
        float capsuleHalfHeight = capsuleHeight * 0.5f;

        glm::vec3 capsulePos = capsuleObj->GetPosition();
        glm::vec3 spherePos  = sphereObj->GetPosition();

        const glm::mat4& sphereTransform  = sphereObj->GetWorldSpaceTransform();
        const glm::mat4& capsuleTransform = capsuleObj->GetWorldSpaceTransform();
        glm::mat4 worldToCapsuleTransform = glm::inverse(capsuleTransform);

        // Transform sphere into capsule space
        glm::mat4 sphereToCapsuleSpaceTransform = worldToCapsuleTransform * sphereTransform;
        glm::vec3 sphereToCapsuleSpacePos       = glm::vec3(sphereToCapsuleSpaceTransform[3]);

        const glm::vec3 capsuleBottom(0, -capsuleHalfHeight, 0);
        const glm::vec3 capsuleTop(0, capsuleHalfHeight, 0);

        // Compute the point on the inner capsule segment that is the closes to centre of sphere
        const glm::vec3 closestPointOnSegment = Maths::ComputeClosestPointOnSegment(capsuleBottom, capsuleTop, sphereToCapsuleSpacePos);

        // Compute the distance between the sphere center and the closest point on the segment
        glm::vec3 sphereCenterToSegment         = (closestPointOnSegment - sphereToCapsuleSpacePos);
        const float sphereSegmentDistanceSquare = glm::length2(sphereCenterToSegment);

        // Compute the sum of the radius of the sphere and the capsule (virtual sphere)
        float sumRadius = sphereRadius + capsuleRadius;

        // If the distance between the sphere center and the closest point on the segment is less than the sum of the radius of the sphere and the capsule,
        // then there is a collision
        if(sphereSegmentDistanceSquare < sumRadius * sumRadius)
        {
            float penetrationDepth;
            glm::vec3 normalWorld;
            glm::vec3 contactPointSphereLocal;
            glm::vec3 contactPointCapsuleLocal;

            // If the sphere center is not on the capsule inner segment
            if(sphereSegmentDistanceSquare > Maths::M_EPSILON)
            {
                float sphereSegmentDistance = std::sqrt(sphereSegmentDistanceSquare);
                sphereCenterToSegment /= sphereSegmentDistance;

                contactPointSphereLocal  = glm::inverse(sphereToCapsuleSpaceTransform) * glm::vec4(sphereToCapsuleSpacePos + sphereCenterToSegment * sphereRadius, 1.0f);
                contactPointCapsuleLocal = closestPointOnSegment - sphereCenterToSegment * capsuleRadius;

                normalWorld = glm::mat4(capsuleObj->GetOrientation()) * glm::vec4(sphereCenterToSegment, 1.0f);

                penetrationDepth = sumRadius - sphereSegmentDistance;

                if(obj1 != sphereObj)
                    normalWorld = -normalWorld;
            }
            else
            {
                // If the sphere center is on the capsule inner segment
                // We take any direction that is orthogonal to the inner capsule segment as a contact normal
                // Capsule inner segment
                glm::vec3 capsuleSegment = glm::normalize(capsuleTop - capsuleBottom);

                glm::vec3 vec1(1, 0, 0);
                glm::vec3 vec2(0, 1, 0);

                // Get the vectors (among vec1 and vec2) that is the most orthogonal to the capsule inner segment (smallest absolute dot product)
                float cosA1 = std::abs(capsuleSegment.x);
                float cosA2 = std::abs(capsuleSegment.y);

                penetrationDepth = sumRadius;

                // We choose as a contact normal, any direction that is perpendicular to the inner capsule segment
                glm::vec3 normalCapsuleSpace = cosA1 < cosA2 ? glm::cross(capsuleSegment, vec1) : glm::cross(capsuleSegment, vec2);
                normalWorld                  = glm::mat4(capsuleObj->GetOrientation()) * glm::vec4(normalCapsuleSpace, 1.0f);

                // Compute the two local contact points
                contactPointSphereLocal  = glm::inverse(sphereToCapsuleSpaceTransform) * glm::vec4(sphereToCapsuleSpacePos + normalCapsuleSpace * sphereRadius, 1.0f);
                contactPointCapsuleLocal = sphereToCapsuleSpacePos - normalCapsuleSpace * capsuleRadius;
            }

            if(penetrationDepth <= 0.0f)
                return false;

            colData.normal       = glm::normalize(normalWorld);
            colData.penetration  = penetrationDepth;
            colData.pointOnPlane = contactPointSphereLocal;

            if(out_coldata)
                *out_coldata = colData;

            return true;
        }

        return false;
    }

    bool CollisionDetection::CheckPolyhedronCapsuleCheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        CollisionShape* complexShape;
        CapsuleCollisionShape* capsuleShape;
        RigidBody3D* complexObj;
        RigidBody3D* capsuleObj;

        if(obj1->GetCollisionShape()->GetType() == CollisionShapeType::CollisionCapsule)
        {
            capsuleObj   = obj1;
            complexShape = shape2;
            complexObj   = obj2;
            capsuleShape = (CapsuleCollisionShape*)shape1;
        }
        else
        {
            capsuleObj   = obj2;
            complexShape = shape1;
            complexObj   = obj1;
            capsuleShape = (CapsuleCollisionShape*)shape2;
        }

        CollisionData cur_colData;
        CollisionData best_colData;
        best_colData.penetration = -FLT_MAX;

        std::vector<glm::vec3>& shapeCollisionAxes      = complexShape->GetCollisionAxes(complexObj);
        std::vector<CollisionEdge>& complex_shape_edges = complexShape->GetEdges(complexObj);

        glm::vec3 p   = GetClosestPointOnEdges(capsuleObj->GetPosition(), complex_shape_edges);
        glm::vec3 p_t = capsuleObj->GetPosition() - p;
        p_t           = glm::normalize(p_t);

        static const int MAX_COLLISION_AXES = 100;
        static glm::vec3 possibleCollisionAxes[MAX_COLLISION_AXES];

        uint32_t possibleCollisionAxesCount = 0;
        for(const glm::vec3& axis : shapeCollisionAxes)
        {
            possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
        }

        AddPossibleCollisionAxis(p_t, possibleCollisionAxes, possibleCollisionAxesCount);

        glm::vec3 capsulePos = capsuleObj->GetPosition();
        glm::vec4 forward    = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        glm::vec3 capsuleDir = capsuleObj->GetWorldSpaceTransform() * forward;

        float capsuleRadius = capsuleShape->GetRadius();
        float capsuleHeight = capsuleShape->GetHeight();

        float capsuleTop    = capsulePos.y + capsuleHeight * 0.5f;
        float capsuleBottom = capsulePos.y - capsuleHeight * 0.5f;

        for(int i = 0; i < shapeCollisionAxes.size(); i++)
        {
            const glm::vec3& axis = shapeCollisionAxes[i];
            if(!CheckCollisionAxis(axis, obj1, obj2, shape1, shape2, &cur_colData))
                return false;

            if(cur_colData.penetration >= best_colData.penetration)
                best_colData = cur_colData;
        }

        if(glm::dot(best_colData.normal, capsuleDir) < 0.0f)
            best_colData.normal = -best_colData.normal;

        if(out_coldata)
            *out_coldata = best_colData;

        return true;
    }

    bool CollisionDetection::CheckCollisionAxis(const glm::vec3& axis, RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION();
        glm::vec3 min1, min2, max1, max2;

        shape1->GetMinMaxVertexOnAxis(obj1, axis, &min1, &max1);
        shape2->GetMinMaxVertexOnAxis(obj2, axis, &min2, &max2);

        float minCorrelation1 = glm::dot(axis, min1);
        float maxCorrelation1 = glm::dot(axis, max1);
        float minCorrelation2 = glm::dot(axis, min2);
        float maxCorrelation2 = glm::dot(axis, max2);

        if(minCorrelation1 <= minCorrelation2 && maxCorrelation1 >= minCorrelation2)
        {
            if(out_coldata != nullptr)
            {
                out_coldata->normal       = axis;
                out_coldata->penetration  = minCorrelation2 - maxCorrelation1;
                out_coldata->pointOnPlane = max1 + out_coldata->normal * out_coldata->penetration;
            }
            return true;
        }

        if(minCorrelation2 <= minCorrelation1 && maxCorrelation2 > minCorrelation1)
        {
            if(out_coldata != nullptr)
            {
                out_coldata->normal       = -axis;
                out_coldata->penetration  = minCorrelation1 - maxCorrelation2;
                out_coldata->pointOnPlane = min1 + out_coldata->normal * out_coldata->penetration;
            }

            return true;
        }

        return false;
    }

    bool CollisionDetection::BuildCollisionManifold(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData& coldata, Manifold* manifold)
    {
        LUMOS_PROFILE_FUNCTION();
        if(!manifold)
            return false;

        ReferencePolygon poly1, poly2;
        shape1->GetIncidentReferencePolygon(obj1, coldata.normal, poly1);
        shape2->GetIncidentReferencePolygon(obj2, -coldata.normal, poly2);

        if(poly1.FaceCount == 0 || poly2.FaceCount == 0)
            return false;
        else if(poly1.FaceCount == 1)
            manifold->AddContact(poly1.Faces[0], poly1.Faces[0] - coldata.normal * coldata.penetration, coldata.normal, coldata.penetration);
        else if(poly2.FaceCount == 1)
            manifold->AddContact(poly2.Faces[0] + coldata.normal * coldata.penetration, poly2.Faces[0], coldata.normal, coldata.penetration);
        else
        {
            bool flipped;
            glm::vec3* incPolygon;
            int incPolygonCount;
            Plane* refAdjPlanes;
            int refAdjPlanesCount;
            Plane refPlane;

            if(glm::abs(glm::dot(coldata.normal, poly1.Normal)) > glm::abs(glm::dot(coldata.normal, poly2.Normal)))
            {
                float planeDist = -(glm::dot(poly1.Faces[0], -poly1.Normal));
                refPlane        = Plane(-poly1.Normal, planeDist);

                refAdjPlanes      = poly1.AdjacentPlanes;
                refAdjPlanesCount = poly1.PlaneCount;
                incPolygon        = poly2.Faces;
                incPolygonCount   = poly2.FaceCount;

                flipped = false;
            }
            else
            {
                float planeDist = -(glm::dot(poly2.Faces[0], -poly2.Normal));
                refPlane        = Plane(-poly2.Normal, planeDist);

                refAdjPlanes      = poly2.AdjacentPlanes;
                refAdjPlanesCount = poly2.PlaneCount;
                incPolygon        = poly1.Faces;
                incPolygonCount   = poly1.FaceCount;

                flipped = true;
            }

            SutherlandHodgesonClipping(incPolygon, incPolygonCount, refAdjPlanesCount, refAdjPlanes, incPolygon, incPolygonCount, false);
            SutherlandHodgesonClipping(incPolygon, incPolygonCount, 1, &refPlane, incPolygon, incPolygonCount, true);

            for(int i = 0; i < incPolygonCount; i++)
            {
                auto& endPoint = incPolygon[i];
                float contact_penetration;
                glm::vec3 globalOnA, globalOnB;

                if(flipped)
                {
                    contact_penetration = -(glm::dot(endPoint, coldata.normal)
                                            - (glm::dot(coldata.normal, poly2.Faces[0])));

                    globalOnA = endPoint + coldata.normal * contact_penetration;
                    globalOnB = endPoint;
                }
                else
                {
                    contact_penetration = glm::dot(endPoint, coldata.normal) - glm::dot(coldata.normal, poly1.Faces[0]);

                    globalOnA = endPoint;
                    globalOnB = endPoint - coldata.normal * contact_penetration;
                }

                manifold->AddContact(globalOnA, globalOnB, coldata.normal, contact_penetration);
            }
        }
        return true;
    }

    glm::vec3 CollisionDetection::GetClosestPointOnEdges(const glm::vec3& target, const std::vector<CollisionEdge>& edges)
    {
        LUMOS_PROFILE_FUNCTION();
        glm::vec3 closest_point, temp_closest_point;
        float closest_distsq = FLT_MAX;

        for(const CollisionEdge& edge : edges)
        {
            glm::vec3 a_t = target - edge.posA;
            glm::vec3 a_b = edge.posB - edge.posA;

            float magnitudeAB = glm::dot(a_b, a_b);        // Magnitude of AB vector (it's length squared)
            float ABAPproduct = glm::dot(a_t, a_b);        // The DOT product of a_to_t and a_to_b
            float distance    = ABAPproduct / magnitudeAB; // The Normalised "distance" from a to your closest point

            if(distance < 0.0f) // Clamp returned point to be on the line, e.g if the closest point is beyond the AB return either A or B as closest points
                temp_closest_point = edge.posA;

            else if(distance > 1)
                temp_closest_point = edge.posB;
            else
                temp_closest_point = edge.posA + a_b * distance;

            glm::vec3 c_t     = target - temp_closest_point;
            float temp_distsq = glm::dot(c_t, c_t);

            if(temp_distsq < closest_distsq)
            {
                closest_distsq = temp_distsq;
                closest_point  = temp_closest_point;
            }
        }

        return closest_point;
    }

    glm::vec3 CollisionDetection::PlaneEdgeIntersection(const Plane& plane, const glm::vec3& start, const glm::vec3& end) const
    {
        glm::vec3 ab = end - start;

        float ab_p = glm::dot(plane.Normal(), ab);

        if(glm::abs(ab_p) > 0.0001f)
        {
            glm::vec3 p_co = plane.Normal() * (-plane.Distance(glm::vec3(0.0f)));

            glm::vec3 w = start - p_co;
            float fac   = -(glm::dot(plane.Normal(), w)) / ab_p;
            ab          = ab * fac;

            return start + ab;
        }

        return start;
    }

    void CollisionDetection::SutherlandHodgesonClipping(glm::vec3* input_polygon, int input_polygon_count, int num_clip_planes, const Plane* clip_planes, glm::vec3* output_polygon, int& output_polygon_count, bool removePoints) const
    {
        LUMOS_PROFILE_FUNCTION();
        if(!output_polygon)
            return;

        glm::vec3 ppPolygon1[8], ppPolygon2[8];
        int inputCount = 0, outputCount = 0;

        glm::vec3 *input = ppPolygon1, *output = ppPolygon2;
        inputCount  = input_polygon_count;
        output      = input_polygon;
        outputCount = inputCount;

        for(int iterations = 0; iterations < num_clip_planes; ++iterations)
        {
            if(outputCount == 0)
                break;

            const Plane& plane = clip_planes[iterations];

            std::swap(input, output);
            inputCount = outputCount;

            outputCount = 0;

            glm::vec3 startPoint = input[inputCount - 1];
            for(int i = 0; i < inputCount; i++)
            {
                const auto& endPoint = input[i];
                bool startInPlane    = plane.IsPointOnPlane(startPoint);
                bool endInPlane      = plane.IsPointOnPlane(endPoint);

                if(removePoints)
                {
                    if(endInPlane)
                        output[outputCount++] = endPoint;
                }
                else
                {
                    // if entire edge is within the clipping plane, keep it as it is
                    if(startInPlane && endInPlane)
                        output[outputCount++] = endPoint;

                    // if edge interesects the clipping plane, cut the edge along clip plane
                    else if(startInPlane && !endInPlane)
                        output[outputCount++] = PlaneEdgeIntersection(plane, startPoint, endPoint);
                    else if(!startInPlane && endInPlane)
                    {
                        output[outputCount++] = PlaneEdgeIntersection(plane, endPoint, startPoint);
                        output[outputCount++] = endPoint;
                    }
                }

                startPoint = endPoint;
            }
        }

        output_polygon       = output;
        output_polygon_count = outputCount;
    }
}
