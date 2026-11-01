#include "Precompiled.h"
#include "CollisionDetection.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/HullCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CapsuleCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/TerrainCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/MeshCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/Hull.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    CollisionDetection::CollisionDetection()
    {
        m_MaxSize                 = CollisionShapeTypeMax | (CollisionShapeTypeMax >> 1);
        m_CollisionCheckFunctions = new CollisionCheckFunc[m_MaxSize];
        for(u32 i = 0; i < m_MaxSize; i++)
            m_CollisionCheckFunctions[i] = &CollisionDetection::CheckPolyhedronCollision;

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

        m_CollisionCheckFunctions[CollisionTerrain | CollisionSphere]  = &CollisionDetection::CheckTerrainCollision;
        m_CollisionCheckFunctions[CollisionTerrain | CollisionCuboid]  = &CollisionDetection::CheckTerrainCollision;
        m_CollisionCheckFunctions[CollisionTerrain | CollisionCapsule] = &CollisionDetection::CheckTerrainCollision;
        m_CollisionCheckFunctions[CollisionTerrain | CollisionPyramid] = &CollisionDetection::CheckTerrainCollision;
        m_CollisionCheckFunctions[CollisionTerrain | CollisionHull]    = &CollisionDetection::CheckTerrainCollision;

        m_CollisionCheckFunctions[CollisionMesh | CollisionSphere]  = &CollisionDetection::CheckMeshCollision;
        m_CollisionCheckFunctions[CollisionMesh | CollisionCuboid]  = &CollisionDetection::CheckMeshCollision;
        m_CollisionCheckFunctions[CollisionMesh | CollisionCapsule] = &CollisionDetection::CheckMeshCollision;
        m_CollisionCheckFunctions[CollisionMesh | CollisionPyramid] = &CollisionDetection::CheckMeshCollision;
        m_CollisionCheckFunctions[CollisionMesh | CollisionHull]    = &CollisionDetection::CheckMeshCollision;
    }

    bool CollisionDetection::CheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        ASSERT(((shape1->GetType() | shape2->GetType()) < m_MaxSize), "Invalid collision func %i, %i, %i, %i", (int)shape1->GetType(), (int)shape2->GetType(), (int)shape1->GetType() | (int)shape2->GetType(), m_MaxSize);
        return CALL_MEMBER_FN(*this, m_CollisionCheckFunctions[shape1->GetType() | shape2->GetType()])(obj1, obj2, shape1, shape2, out_coldata);
    }

    bool CollisionDetection::InvalidCheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LFATAL("Invalid Collision type specified");
        return false;
    }

    bool CollisionDetection::CheckSphereCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        ASSERT(shape1->GetType() == CollisionShapeType::CollisionSphere && shape2->GetType() == CollisionShapeType::CollisionSphere, "Both shapes are not spheres");

        CollisionData colData;
        Vec3 axis = obj2->GetPosition() - obj1->GetPosition();

        float sumRadii        = ((SphereCollisionShape*)shape1)->GetRadius() + ((SphereCollisionShape*)shape2)->GetRadius();
        float sumRadiiSquared = sumRadii * sumRadii;
        float distSquared     = Maths::Length2(axis);
        if(distSquared > sumRadiiSquared)
            return false;

        colData.normal       = axis.Normalised();
        colData.penetration  = sumRadii - Maths::Sqrt(distSquared);
        // Contact point on sphere1's surface
        colData.pointOnPlane = obj1->GetPosition() + colData.normal * ((SphereCollisionShape*)shape1)->GetRadius();

        if(out_coldata)
            *out_coldata = colData;

        return true;
    }

    void AddPossibleCollisionAxis(Vec3& axis, Vec3* possibleCollisionAxes, uint32_t& possibleCollisionAxesCount, uint32_t maxAxes)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        if(Maths::Length2(axis) < Maths::M_EPSILON)
            return;

        axis.Normalise();

        float value = (1.0f - Maths::M_EPSILON);

        for(uint32_t i = 0; i < possibleCollisionAxesCount; i++)
        {
            const Vec3& p_axis = possibleCollisionAxes[i];
            if(Maths::Abs(Maths::Dot(axis, p_axis)) >= value)
                return;
        }

        // Bounds check to prevent buffer overflow
        if(possibleCollisionAxesCount >= maxAxes)
        {
            LWARN("Collision axes limit reached (%u), skipping axis", maxAxes);
            return;
        }

        possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
    }

    bool CollisionDetection::CheckPolyhedronSphereCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        ASSERT(shape1->GetType() == CollisionShapeType::CollisionSphere || shape2->GetType() == CollisionShapeType::CollisionSphere, "No sphere collision shape");

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

        TDArray<Vec3>& shapeCollisionAxes           = complexShape->GetCollisionAxes(complexObj);
        TDArray<CollisionEdge>& complex_shape_edges = complexShape->GetEdges(complexObj);

        Vec3 p   = GetClosestPointOnEdges(sphereObj->GetPosition(), complex_shape_edges);
        Vec3 p_t = sphereObj->GetPosition() - p;
        p_t.Normalise();

        const int MAX_COLLISION_AXES = 100;
        Vec3 possibleCollisionAxes[MAX_COLLISION_AXES];

        uint32_t possibleCollisionAxesCount = 0;
        for(const Vec3& axis : shapeCollisionAxes)
        {
            if(possibleCollisionAxesCount >= MAX_COLLISION_AXES)
            {
                LWARN("Collision axes limit reached in CheckPolyhedronSphereCollision");
                break;
            }
            possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
        }

        AddPossibleCollisionAxis(p_t, possibleCollisionAxes, possibleCollisionAxesCount, MAX_COLLISION_AXES);

        for(uint32_t i = 0; i < possibleCollisionAxesCount; i++)
        {
            const Vec3& axis = possibleCollisionAxes[i];
            // Early termination: if separation axis found, no collision exists
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
        LUMOS_PROFILE_FUNCTION_LOW();
        CollisionData cur_colData;
        CollisionData best_colData;
        best_colData.penetration = -FLT_MAX;

        TDArray<Vec3>& shape1CollisionAxes         = shape1->GetCollisionAxes(obj1);
        TDArray<Vec3>& shape2PossibleCollisionAxes = shape2->GetCollisionAxes(obj2);

        const int MAX_COLLISION_AXES = 100;
        Vec3 possibleCollisionAxes[MAX_COLLISION_AXES];

        uint32_t possibleCollisionAxesCount = 0;
        for(const Vec3& axis : shape1CollisionAxes)
        {
            if(possibleCollisionAxesCount >= MAX_COLLISION_AXES)
            {
                LWARN("Collision axes limit reached in CheckPolyhedronCollision (shape1)");
                break;
            }
            possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
        }

        for(const Vec3& axis : shape2PossibleCollisionAxes)
        {
            if(possibleCollisionAxesCount >= MAX_COLLISION_AXES)
            {
                LWARN("Collision axes limit reached in CheckPolyhedronCollision (shape2)");
                break;
            }
            possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
        }

        TDArray<CollisionEdge>& shape1_edges = shape1->GetEdges(obj1);
        TDArray<CollisionEdge>& shape2_edges = shape2->GetEdges(obj2);

        for(uint32_t i = 0; i < possibleCollisionAxesCount; i++)
        {
            const Vec3& axis = possibleCollisionAxes[i];
            // Early termination: if separation axis found, no collision exists
            if(!CheckCollisionAxis(axis, obj1, obj2, shape1, shape2, &cur_colData))
                return false;

            if(cur_colData.penetration >= best_colData.penetration)
                best_colData = cur_colData;
        }

        if(out_coldata)
            *out_coldata = best_colData;

        return true;
    }

    float PlaneSegmentIntersection(const Vec3& segA, const Vec3& segB, const float planeD, const Vec3& planeNormal)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        float t = -1.0f;

        const float nDotAB = Maths::Dot(planeNormal, segB - segA);

        // If the segment is not parallel to the plane
        if(Maths::Abs(nDotAB) > Maths::M_EPSILON)
        {
            t = (planeD - Maths::Dot(planeNormal, segA)) / nDotAB;
        }

        return t;
    }

    float PointToLineDistance(const Vec3& linePointA, const Vec3& linePointB, const Vec3& point)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        float distAB = Maths::Length(linePointB - linePointA);

        if(distAB < Maths::M_EPSILON)
        {
            return Maths::Length(point - linePointA);
        }

        return (Maths::Length(Maths::Cross((point - linePointA), (point - linePointB)))) / distAB;
    }

    bool CollisionDetection::CheckCapsuleCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        CapsuleCollisionShape* capsuleShape1 = static_cast<CapsuleCollisionShape*>(shape1);
        CapsuleCollisionShape* capsuleShape2 = static_cast<CapsuleCollisionShape*>(shape2);

        CollisionData best_colData;
        best_colData.penetration = -FLT_MAX;

        float capsule1Radius = capsuleShape1->GetRadius();
        float capsule2Radius = capsuleShape2->GetRadius();

        Vec3 p1 = obj1->GetPosition();
        Vec3 p2 = obj2->GetPosition();

        float height1 = capsuleShape1->GetHeight();
        float height2 = capsuleShape2->GetHeight();

        float capsule1HalfHeight = height1 * 0.5f;
        float capsule2HalfHeight = height2 * 0.5f;

        float radiusSum = capsule1Radius + capsule2Radius;

        Mat4 capsule1ToCapsule2SpaceTransform = Mat4::Inverse(obj2->GetWorldSpaceTransform()) * obj1->GetWorldSpaceTransform();

        Vec3 capsule1SegA(0, -capsule1HalfHeight, 0);
        Vec3 capsule1SegB(0, capsule1HalfHeight, 0);
        capsule1SegA = capsule1ToCapsule2SpaceTransform * capsule1SegA;
        capsule1SegB = capsule1ToCapsule2SpaceTransform * capsule1SegB;

        Vec3 capsule2SegA(0, -capsule2HalfHeight, 0);
        Vec3 capsule2SegB(0, capsule2HalfHeight, 0);

        // The two inner capsule segments
        const Vec3 seg1  = capsule1SegB - capsule1SegA;
        const Vec3 seg2  = capsule2SegB - capsule2SegA;
        
        // Check for degenerate (zero-length) segments
        float seg1LengthSq = Maths::Length2(seg1);
        float seg2LengthSq = Maths::Length2(seg2);
        if(seg1LengthSq < Maths::M_EPSILON * Maths::M_EPSILON || seg2LengthSq < Maths::M_EPSILON * Maths::M_EPSILON)
        {
            // Degenerate capsule(s), treat as sphere-sphere collision
            Vec3 axis = p2 - p1;
            float distSq = Maths::Length2(axis);
            if(distSq > radiusSum * radiusSum)
                return false;
                
            float dist = Maths::Sqrt(distSq);
            best_colData.normal = dist > Maths::M_EPSILON ? axis / dist : Vec3(0.0f, 1.0f, 0.0f);
            best_colData.penetration = radiusSum - dist;
            best_colData.pointOnPlane = p1 + axis * 0.5f;
            
            if(out_coldata)
                *out_coldata = best_colData;
            return true;
        }
        
        bool areParallel = Maths::AreVectorsParallel(seg1, seg2);

        if(areParallel)
        {
            float segmentsPerpendicularDistance = PointToLineDistance(capsule1SegA, capsule1SegB, capsule2SegA);
            if(segmentsPerpendicularDistance > radiusSum)
                return false;

            // Calculate plane distances correctly using normalized direction
            Vec3 seg1Norm = seg1.Normalised();
            float d1 = Maths::Dot(seg1Norm, Vec3(capsule1SegA));
            float d2 = Maths::Dot(seg1Norm, Vec3(capsule1SegB));

            float t1 = PlaneSegmentIntersection(capsule2SegB, capsule2SegA, d1, seg1Norm);
            float t2 = PlaneSegmentIntersection(capsule2SegA, capsule2SegB, d2, seg1Norm);

            if(t1 > 0.0f && t2 > 0.0f)
            {
                // Clip the inner segment of capsule 2
                if(t1 > 1.0f)
                    t1 = 1.0f;
                const Vec3 clipPointA = capsule2SegB - t1 * seg2;
                if(t2 > 1.0f)
                    t2 = 1.0f;
                const Vec3 clipPointB = capsule2SegA + t2 * seg2;

                // Project point capsule2SegA onto line of inner segment of capsule 1
                const Vec3 seg1Normalized    = seg1.Normalised();
                Vec3 pointOnInnerSegCapsule1 = Vec3(capsule1SegA) + Maths::Dot(seg1Normalized, Vec3(capsule2SegA - capsule1SegA)) * seg1Normalized;

                Vec3 normalCapsule2SpaceNormalized;
                Vec3 segment1ToSegment2;

                // If the inner capsule segments perpendicular distance is not zero (the inner segments are not overlapping)
                if(segmentsPerpendicularDistance > Maths::M_EPSILON)
                {
                    // Compute a perpendicular vector from segment 1 to segment 2
                    segment1ToSegment2            = (capsule2SegA - pointOnInnerSegCapsule1);
                    normalCapsule2SpaceNormalized = segment1ToSegment2.Normalised();
                }
                else
                {
                    Vec3 vec1(1, 0, 0);
                    Vec3 vec2(0, 1, 0);

                    Vec3 seg2Normalized = seg2.Normalised();

                    float cosA1        = Maths::Abs(seg2Normalized.x);
                    float cosA2        = Maths::Abs(seg2Normalized.y);
                    segment1ToSegment2 = Vec3(0.0f, 0.0f, 0.0f);

                    normalCapsule2SpaceNormalized = cosA1 < cosA2 ? Maths::Cross(seg2Normalized, vec1) : Maths::Cross(seg2Normalized, vec2);
                }

                Mat4 capsule2ToCapsule1SpaceTransform = capsule1ToCapsule2SpaceTransform.Inverse();
                const Vec3 contactPointACapsule1Local = capsule2ToCapsule1SpaceTransform * Vec4(clipPointA - segment1ToSegment2 + normalCapsule2SpaceNormalized * capsule1Radius, 1.0f);
                const Vec3 contactPointBCapsule1Local = capsule2ToCapsule1SpaceTransform * Vec4(clipPointB - segment1ToSegment2 + normalCapsule2SpaceNormalized * capsule1Radius, 1.0f);
                const Vec3 contactPointACapsule2Local = clipPointA - normalCapsule2SpaceNormalized * capsule2Radius;
                const Vec3 contactPointBCapsule2Local = clipPointB - normalCapsule2SpaceNormalized * capsule2Radius;

                float penetrationDepth = radiusSum - segmentsPerpendicularDistance;

                Vec3 normalWorld = Mat4(obj2->GetOrientation()) * Vec4(normalCapsule2SpaceNormalized, 1.0f);

                // Ensure normal points from obj1 to obj2 (using contact points for consistency)
                Vec3 contactPoint1World = obj1->GetWorldSpaceTransform() * Vec4(contactPointACapsule1Local, 1.0f);
                Vec3 contactPoint2World = obj2->GetWorldSpaceTransform() * Vec4(contactPointACapsule2Local, 1.0f);
                Vec3 contactDelta = contactPoint2World - contactPoint1World;
                
                if(Maths::Dot(normalWorld, contactDelta) < 0.0f)
                {
                    normalWorld = -normalWorld;
                }

                best_colData.normal       = normalWorld.Normalised();
                best_colData.penetration  = penetrationDepth;
                best_colData.pointOnPlane = obj1->GetWorldSpaceTransform() * Vec4(contactPointACapsule1Local, 1.0f);

                if(out_coldata)
                    *out_coldata = best_colData;

                return true;
            }
        }

        Vec3 closestPointCapsule1Seg;
        Vec3 closestPointCapsule2Seg;
        Maths::ClosestPointBetweenTwoSegments(capsule1SegA, capsule1SegB, capsule2SegA, capsule2SegB,
                                              closestPointCapsule1Seg, closestPointCapsule2Seg);

        Vec3 closestPointsSeg1ToSeg2            = (closestPointCapsule2Seg - closestPointCapsule1Seg);
        const float closestPointsDistanceSquare = Maths::Length2(closestPointsSeg1ToSeg2);

        if(closestPointsDistanceSquare < radiusSum * radiusSum)
        {
            if(closestPointsDistanceSquare > Maths::M_EPSILON)
            {
                float closestPointsDistance = Maths::Sqrt(closestPointsDistanceSquare);
                closestPointsSeg1ToSeg2 /= closestPointsDistance;

                const Vec3 contactPointCapsule1Local = Mat4::Inverse(capsule1ToCapsule2SpaceTransform) * (closestPointCapsule1Seg + closestPointsSeg1ToSeg2 * capsule1Radius);
                const Vec3 contactPointCapsule2Local = closestPointCapsule2Seg - closestPointsSeg1ToSeg2 * capsule2Radius;

                Vec3 normalWorld = Mat4(obj2->GetOrientation()) * Vec4(closestPointsSeg1ToSeg2, 1.0f);

                float penetrationDepth = radiusSum - closestPointsDistance;

                // Ensure normal points from obj1 to obj2 (should point from capsule1 to capsule2)
                // closestPointsSeg1ToSeg2 already points from seg1 to seg2 in capsule2 space
                // After rotation to world space, verify it points from obj1 to obj2
                Vec3 obj1ToObj2 = p2 - p1;
                if(Maths::Dot(normalWorld, obj1ToObj2) < 0.0f)
                {
                    normalWorld = -normalWorld;
                }

                best_colData.normal       = normalWorld.Normalised();
                best_colData.penetration  = penetrationDepth;
                best_colData.pointOnPlane = obj1->GetWorldSpaceTransform() * Vec4(contactPointCapsule1Local, 1.0f);

                if(out_coldata)
                    *out_coldata = best_colData;

                return true;
            }
            else
            {
                if(areParallel)
                {
                    float squareDistCapsule2PointToCapsuleSegA = Maths::Length2((capsule1SegA - closestPointCapsule2Seg));

                    Vec3 capsule1SegmentMostExtremePoint = squareDistCapsule2PointToCapsuleSegA > Maths::M_EPSILON ? capsule1SegA : capsule1SegB;
                    Vec3 normalCapsuleSpace2             = (closestPointCapsule2Seg - capsule1SegmentMostExtremePoint);
                    normalCapsuleSpace2                  = normalCapsuleSpace2.Normalised();

                    const Vec3 contactPointCapsule1Local = Mat4::Inverse(capsule1ToCapsule2SpaceTransform) * (closestPointCapsule1Seg + normalCapsuleSpace2 * capsule1Radius);
                    const Vec3 contactPointCapsule2Local = closestPointCapsule2Seg - normalCapsuleSpace2 * capsule2Radius;

                    Vec3 normalWorld = Mat4(obj2->GetOrientation()) * Vec4(normalCapsuleSpace2, 1.0f);

                    // Ensure normal points from obj1 to obj2
                    Vec3 obj1ToObj2 = p2 - p1;
                    if(Maths::Length2(obj1ToObj2) > Maths::M_EPSILON)
                    {
                        // Use center-to-center if available
                        if(Maths::Dot(normalWorld, obj1ToObj2) < 0.0f)
                        {
                            normalWorld = -normalWorld;
                        }
                    }
                    else
                    {
                        // Centers are nearly coincident, use contact point direction
                        Vec3 contactPoint1World = obj1->GetWorldSpaceTransform() * Vec4(contactPointCapsule1Local, 1.0f);
                        Vec3 contactPoint2World = obj2->GetWorldSpaceTransform() * Vec4(contactPointCapsule2Local, 1.0f);
                        Vec3 contactDelta = contactPoint2World - contactPoint1World;
                        if(Maths::Dot(normalWorld, contactDelta) < 0.0f)
                        {
                            normalWorld = -normalWorld;
                        }
                    }

                    best_colData.normal       = normalWorld.Normalised();
                    best_colData.penetration  = radiusSum;
                    best_colData.pointOnPlane = obj1->GetWorldSpaceTransform() * Vec4(contactPointCapsule1Local, 1.0f);

                    if(out_coldata)
                        *out_coldata = best_colData;

                    return true;
                }
                else
                {
                    Vec3 crossResult = Maths::Cross(seg1, seg2);
                    Vec3 normalCapsuleSpace2;
                    if(Maths::Length2(crossResult) < Maths::M_EPSILON)
                        normalCapsuleSpace2 = Maths::PerpendicularVector(seg1);
                    else
                        normalCapsuleSpace2 = crossResult.Normalised();

                    const Vec3 contactPointCapsule1Local = Mat4::Inverse(capsule1ToCapsule2SpaceTransform) * (closestPointCapsule1Seg + normalCapsuleSpace2 * capsule1Radius);
                    const Vec3 contactPointCapsule2Local = closestPointCapsule2Seg - normalCapsuleSpace2 * capsule2Radius;

                    Vec3 normalWorld = Mat4(obj2->GetOrientation()) * Vec4(normalCapsuleSpace2, 1.0f);

                    // Ensure normal points from obj1 to obj2
                    Vec3 obj1ToObj2 = p2 - p1;
                    if(Maths::Length2(obj1ToObj2) > Maths::M_EPSILON)
                    {
                        if(Maths::Dot(normalWorld, obj1ToObj2) < 0.0f)
                        {
                            normalWorld = -normalWorld;
                        }
                    }
                    else
                    {
                        // Centers coincident, use contact points
                        Vec3 contactPoint1World = obj1->GetWorldSpaceTransform() * Vec4(contactPointCapsule1Local, 1.0f);
                        Vec3 contactPoint2World = obj2->GetWorldSpaceTransform() * Vec4(contactPointCapsule2Local, 1.0f);
                        Vec3 contactDelta = contactPoint2World - contactPoint1World;
                        if(Maths::Dot(normalWorld, contactDelta) < 0.0f)
                        {
                            normalWorld = -normalWorld;
                        }
                    }

                    best_colData.normal       = normalWorld.Normalised();
                    best_colData.penetration  = radiusSum;
                    best_colData.pointOnPlane = obj1->GetWorldSpaceTransform() * Vec4(contactPointCapsule1Local, 1.0f);

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
        LUMOS_PROFILE_FUNCTION_LOW();
        ASSERT(shape1->GetType() == CollisionShapeType::CollisionSphere || shape2->GetType() == CollisionShapeType::CollisionSphere, "Both shapes are not spheres");

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

        Vec3 capsulePos = capsuleObj->GetPosition();
        Vec3 spherePos  = sphereObj->GetPosition();

        const Mat4& sphereTransform  = sphereObj->GetWorldSpaceTransform();
        const Mat4& capsuleTransform = capsuleObj->GetWorldSpaceTransform();
        Mat4 worldToCapsuleTransform = Mat4::Inverse(capsuleTransform);

        // Transform sphere into capsule space
        Mat4 sphereToCapsuleSpaceTransform = worldToCapsuleTransform * sphereTransform;
        Vec3 sphereToCapsuleSpacePos       = Vec3(sphereToCapsuleSpaceTransform.GetPositionVector());

        const Vec3 capsuleBottom(0, -capsuleHalfHeight, 0);
        const Vec3 capsuleTop(0, capsuleHalfHeight, 0);

        // Compute the point on the inner capsule segment that is the closes to centre of sphere
        const Vec3 closestPointOnSegment = Maths::ComputeClosestPointOnSegment(capsuleBottom, capsuleTop, sphereToCapsuleSpacePos);

        // Compute the distance between the sphere center and the closest point on the segment
        Vec3 sphereCenterToSegment              = (closestPointOnSegment - sphereToCapsuleSpacePos);
        const float sphereSegmentDistanceSquare = Maths::Length2(sphereCenterToSegment);

        // Compute the sum of the radius of the sphere and the capsule (virtual sphere)
        float sumRadius = sphereRadius + capsuleRadius;

        // If the distance between the sphere center and the closest point on the segment is less than the sum of the radius of the sphere and the capsule,
        // then there is a collision
        if(sphereSegmentDistanceSquare < sumRadius * sumRadius)
        {
            float penetrationDepth;
            Vec3 normalWorld;
            Vec3 contactPointSphereWorld;
            Vec3 contactPointCapsuleWorld;

            // If the sphere center is not on the capsule inner segment
            if(sphereSegmentDistanceSquare > Maths::M_EPSILON)
            {
                float sphereSegmentDistance = Maths::Sqrt(sphereSegmentDistanceSquare);
                sphereCenterToSegment /= sphereSegmentDistance;

                // Calculate contact points in world space
                contactPointCapsuleWorld = capsuleTransform * Vec4(closestPointOnSegment - sphereCenterToSegment * capsuleRadius, 1.0f);
                contactPointSphereWorld  = capsuleTransform * Vec4(sphereToCapsuleSpacePos + sphereCenterToSegment * sphereRadius, 1.0f);

                // Normal in capsule space points from segment to sphere center
                // Transform to world space
                normalWorld = Mat4(capsuleObj->GetOrientation()) * Vec4(sphereCenterToSegment, 1.0f);

                penetrationDepth = sumRadius - sphereSegmentDistance;

                // Ensure normal always points from obj1 to obj2
                Vec3 obj1ToObj2 = obj2->GetPosition() - obj1->GetPosition();
                if(Maths::Dot(normalWorld, obj1ToObj2) < 0.0f)
                {
                    normalWorld = -normalWorld;
                }
            }
            else
            {
                // If the sphere center is on the capsule inner segment
                // We take any direction that is orthogonal to the inner capsule segment as a contact normal
                // Capsule inner segment
                Vec3 capsuleSegment = (capsuleTop - capsuleBottom).Normalised();

                Vec3 vec1(1, 0, 0);
                Vec3 vec2(0, 1, 0);

                // Get the vectors (among vec1 and vec2) that is the most orthogonal to the capsule inner segment (smallest absolute dot product)
                float cosA1 = Maths::Abs(capsuleSegment.x);
                float cosA2 = Maths::Abs(capsuleSegment.y);

                penetrationDepth = sumRadius;

                // We choose as a contact normal, any direction that is perpendicular to the inner capsule segment
                Vec3 normalCapsuleSpace = cosA1 < cosA2 ? Maths::Cross(capsuleSegment, vec1) : Maths::Cross(capsuleSegment, vec2);
                normalWorld             = Mat4(capsuleObj->GetOrientation()) * Vec4(normalCapsuleSpace, 1.0f);

                // Compute contact points in world space
                contactPointSphereWorld  = capsuleTransform * Vec4(sphereToCapsuleSpacePos + normalCapsuleSpace * sphereRadius, 1.0f);
                contactPointCapsuleWorld = capsuleTransform * Vec4(sphereToCapsuleSpacePos - normalCapsuleSpace * capsuleRadius, 1.0f);
            }

            if(penetrationDepth <= 0.0f)
                return false;

            colData.normal       = normalWorld.Normalised();
            colData.penetration  = penetrationDepth;
            // Use the contact point in world space
            colData.pointOnPlane = contactPointSphereWorld;

            if(out_coldata)
                *out_coldata = colData;

            return true;
        }

        return false;
    }

    bool CollisionDetection::CheckPolyhedronCapsuleCheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
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

        TDArray<Vec3>& shapeCollisionAxes           = complexShape->GetCollisionAxes(complexObj);
        TDArray<CollisionEdge>& complex_shape_edges = complexShape->GetEdges(complexObj);

        Vec3 p   = GetClosestPointOnEdges(capsuleObj->GetPosition(), complex_shape_edges);
        Vec3 p_t = capsuleObj->GetPosition() - p;
        p_t.Normalise();

        const int MAX_COLLISION_AXES = 100;
        Vec3 possibleCollisionAxes[MAX_COLLISION_AXES];

        uint32_t possibleCollisionAxesCount = 0;
        for(const Vec3& axis : shapeCollisionAxes)
        {
            if(possibleCollisionAxesCount >= MAX_COLLISION_AXES)
            {
                LWARN("Collision axes limit reached in CheckPolyhedronCapsuleCheckCollision");
                break;
            }
            possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
        }

        AddPossibleCollisionAxis(p_t, possibleCollisionAxes, possibleCollisionAxesCount, MAX_COLLISION_AXES);

        // Add axes from capsule endpoints to closest edge points
        float capsuleHalfHeight = capsuleShape->GetHeight() * 0.5f;
        const Mat4& capsuleWS = capsuleObj->GetWorldSpaceTransform();
        Vec3 capsuleTop = capsuleWS * Vec4(0, capsuleHalfHeight, 0, 1.0f);
        Vec3 capsuleBottom = capsuleWS * Vec4(0, -capsuleHalfHeight, 0, 1.0f);

        Vec3 pTop = GetClosestPointOnEdges(capsuleTop, complex_shape_edges);
        Vec3 axisTop = capsuleTop - pTop;
        AddPossibleCollisionAxis(axisTop, possibleCollisionAxes, possibleCollisionAxesCount, MAX_COLLISION_AXES);

        Vec3 pBottom = GetClosestPointOnEdges(capsuleBottom, complex_shape_edges);
        Vec3 axisBottom = capsuleBottom - pBottom;
        AddPossibleCollisionAxis(axisBottom, possibleCollisionAxes, possibleCollisionAxesCount, MAX_COLLISION_AXES);

        for(uint32_t i = 0; i < possibleCollisionAxesCount; i++)
        {
            const Vec3& axis = possibleCollisionAxes[i];
            // Early termination: if separation axis found, no collision exists
            if(!CheckCollisionAxis(axis, obj1, obj2, shape1, shape2, &cur_colData))
                return false;

            if(cur_colData.penetration >= best_colData.penetration)
                best_colData = cur_colData;
        }

        // Ensure normal always points from obj1 to obj2 for consistency
        Vec3 obj1ToObj2 = obj2->GetPosition() - obj1->GetPosition();
        if(Maths::Dot(best_colData.normal, obj1ToObj2) < 0.0f)
        {
            best_colData.normal = -best_colData.normal;
        }

        if(out_coldata)
            *out_coldata = best_colData;

        return true;
    }

    bool CollisionDetection::CheckTerrainCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION_LOW();

        // Determine which body is terrain
        TerrainCollisionShape* terrainShape;
        RigidBody3D* terrainObj;
        RigidBody3D* otherObj;
        CollisionShape* otherShape;

        if(shape1->GetType() == CollisionShapeType::CollisionTerrain)
        {
            terrainShape = static_cast<TerrainCollisionShape*>(shape1);
            terrainObj   = obj1;
            otherObj     = obj2;
            otherShape   = shape2;
        }
        else
        {
            terrainShape = static_cast<TerrainCollisionShape*>(shape2);
            terrainObj   = obj2;
            otherObj     = obj1;
            otherShape   = shape1;
        }

        Vec3 terrainPos = terrainObj->GetPosition();
        Vec3 otherPos   = otherObj->GetPosition();

        // Local position relative to terrain origin
        Vec3 localPos = otherPos - terrainPos;

        // Collect sample points based on other shape type
        const int MAX_SAMPLE_POINTS = 16;
        Vec3 samplePoints[MAX_SAMPLE_POINTS];
        float sampleRadii[MAX_SAMPLE_POINTS];
        int sampleCount = 0;

        switch(otherShape->GetType())
        {
        case CollisionShapeType::CollisionSphere:
        {
            auto* sphere         = static_cast<SphereCollisionShape*>(otherShape);
            float r              = sphere->GetRadius();
            samplePoints[0]     = localPos;
            sampleRadii[0]      = r;
            sampleCount         = 1;
            break;
        }
        case CollisionShapeType::CollisionCapsule:
        {
            auto* capsule        = static_cast<CapsuleCollisionShape*>(otherShape);
            float r              = capsule->GetRadius();
            float halfH          = capsule->GetHeight() * 0.5f;
            const Mat4& ws       = otherObj->GetWorldSpaceTransform();
            Vec3 top             = Vec3(ws * Vec4(0, halfH, 0, 1.0f)) - terrainPos;
            Vec3 center          = localPos;
            Vec3 bottom          = Vec3(ws * Vec4(0, -halfH, 0, 1.0f)) - terrainPos;
            samplePoints[0]     = top;
            sampleRadii[0]      = r;
            samplePoints[1]     = center;
            sampleRadii[1]      = r;
            samplePoints[2]     = bottom;
            sampleRadii[2]      = r;
            sampleCount         = 3;
            break;
        }
        default:
        {
            // For cuboids, hulls, pyramids: sample AABB corners + center
            Vec3 minV, maxV;
            const Vec3 axes[3] = { Vec3(1,0,0), Vec3(0,1,0), Vec3(0,0,1) };
            Vec3 shapeMin(FLT_MAX), shapeMax(-FLT_MAX);

            for(int a = 0; a < 3; a++)
            {
                Vec3 mn, mx;
                otherShape->GetMinMaxVertexOnAxis(otherObj, axes[a], &mn, &mx);
                float minP = Maths::Dot(mn, axes[a]);
                float maxP = Maths::Dot(mx, axes[a]);
                if(a == 0) { shapeMin.x = minP; shapeMax.x = maxP; }
                else if(a == 1) { shapeMin.y = minP; shapeMax.y = maxP; }
                else { shapeMin.z = minP; shapeMax.z = maxP; }
            }

            Vec3 center = (shapeMin + shapeMax) * 0.5f;
            samplePoints[0] = center - terrainPos;
            sampleRadii[0]  = 0.0f;
            sampleCount     = 1;

            // 8 AABB corners
            for(int i = 0; i < 8 && sampleCount < MAX_SAMPLE_POINTS; i++)
            {
                Vec3 corner;
                corner.x = (i & 1) ? shapeMax.x : shapeMin.x;
                corner.y = (i & 2) ? shapeMax.y : shapeMin.y;
                corner.z = (i & 4) ? shapeMax.z : shapeMin.z;
                samplePoints[sampleCount] = corner - terrainPos;
                sampleRadii[sampleCount]  = 0.0f;
                sampleCount++;
            }
            break;
        }
        }

        // Find deepest penetration across all sample points
        float bestPenetration = 0.0f;
        Vec3 bestNormal;
        Vec3 bestContact;
        bool found = false;

        for(int i = 0; i < sampleCount; i++)
        {
            Vec3 contact, normal;
            float penetration;
            if(terrainShape->GetContactPoint(samplePoints[i], sampleRadii[i], contact, normal, penetration))
            {
                if(!found || penetration > bestPenetration)
                {
                    bestPenetration = penetration;
                    bestNormal      = normal;
                    bestContact     = contact + terrainPos; // Back to world space
                    found           = true;
                }
            }
        }

        if(!found)
            return false;

        if(out_coldata)
        {
            if(shape2->GetType() == CollisionShapeType::CollisionTerrain)
                bestNormal = -bestNormal;

            out_coldata->normal       = bestNormal;
            out_coldata->penetration  = -bestPenetration; // Negative = overlapping
            out_coldata->pointOnPlane = bestContact;
        }

        return true;
    }

    // ---- triangle mesh ---------------------------------------------------------------
    // All in the mesh's local space. Normals point from the mesh toward the other body.
    struct MeshContact
    {
        Vec3 Point; // on the mesh surface
        Vec3 Normal;
        float Depth;
        bool Face; // touching the inside of a triangle, not one of its edges
    };

    static constexpr uint32_t kMaxMeshContacts = 4;

    // Narrowphase asks for the manifold straight after the check, on the same thread.
    struct MeshContactCache
    {
        RigidBody3D* MeshBody  = nullptr;
        RigidBody3D* OtherBody = nullptr;
        Vec3 OtherPosition;
        MeshContact Contacts[kMaxMeshContacts];
        uint32_t Count = 0;
    };
    static thread_local MeshContactCache t_MeshContacts;

    static Vec3 ClosestPointOnTriangle(const Vec3& p, const MeshTriangle& tri, bool& interior)
    {
        interior      = false;
        const Vec3 ab = tri.v1 - tri.v0;
        const Vec3 ac = tri.v2 - tri.v0;
        const Vec3 ap = p - tri.v0;
        const float d1 = Maths::Dot(ab, ap), d2 = Maths::Dot(ac, ap);
        if(d1 <= 0.0f && d2 <= 0.0f)
            return tri.v0;

        const Vec3 bp  = p - tri.v1;
        const float d3 = Maths::Dot(ab, bp), d4 = Maths::Dot(ac, bp);
        if(d3 >= 0.0f && d4 <= d3)
            return tri.v1;

        const float vc = d1 * d4 - d3 * d2;
        if(vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
            return tri.v0 + ab * (d1 / (d1 - d3));

        const Vec3 cp  = p - tri.v2;
        const float d5 = Maths::Dot(ab, cp), d6 = Maths::Dot(ac, cp);
        if(d6 >= 0.0f && d5 <= d6)
            return tri.v2;

        const float vb = d5 * d2 - d1 * d6;
        if(vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
            return tri.v0 + ac * (d2 / (d2 - d6));

        const float va = d3 * d6 - d5 * d4;
        if(va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
            return tri.v1 + (tri.v2 - tri.v1) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));

        interior          = true;
        const float denom = 1.0f / (va + vb + vc);
        return tri.v0 + ab * (vb * denom) + ac * (vc * denom);
    }

    static void ClosestPointsOnSegments(const Vec3& p1, const Vec3& q1, const Vec3& p2, const Vec3& q2, Vec3& c1, Vec3& c2)
    {
        const Vec3 d1 = q1 - p1, d2 = q2 - p2, r = p1 - p2;
        const float a = Maths::Dot(d1, d1), e = Maths::Dot(d2, d2), f = Maths::Dot(d2, r);
        float s = 0.0f, t = 0.0f;
        if(a <= 1e-12f && e <= 1e-12f)
        {
        }
        else if(a <= 1e-12f)
            t = Maths::Clamp(f / e, 0.0f, 1.0f);
        else
        {
            const float c = Maths::Dot(d1, r);
            if(e <= 1e-12f)
                s = Maths::Clamp(-c / a, 0.0f, 1.0f);
            else
            {
                const float b     = Maths::Dot(d1, d2);
                const float denom = a * e - b * b;
                s                 = denom > 1e-12f ? Maths::Clamp((b * f - c * e) / denom, 0.0f, 1.0f) : 0.0f;
                t                 = (b * s + f) / e;
                if(t < 0.0f)
                {
                    t = 0.0f;
                    s = Maths::Clamp(-c / a, 0.0f, 1.0f);
                }
                else if(t > 1.0f)
                {
                    t = 1.0f;
                    s = Maths::Clamp((b - c) / a, 0.0f, 1.0f);
                }
            }
        }
        c1 = p1 + d1 * s;
        c2 = p2 + d2 * t;
    }

    // Where the segment p -> q passes through the triangle, either side.
    static bool SegmentThroughTriangle(const Vec3& p, const Vec3& q, const MeshTriangle& tri, Vec3& outPoint)
    {
        const float dp = Maths::Dot(tri.Normal, p - tri.v0);
        const float dq = Maths::Dot(tri.Normal, q - tri.v0);
        if((dp > 0.0f) == (dq > 0.0f) || dp == dq)
            return false;

        const Vec3 x = p + (q - p) * (dp / (dp - dq));
        if(Maths::Dot(Maths::Cross(tri.v1 - tri.v0, x - tri.v0), tri.Normal) < 0.0f
            || Maths::Dot(Maths::Cross(tri.v2 - tri.v1, x - tri.v1), tri.Normal) < 0.0f
            || Maths::Dot(Maths::Cross(tri.v0 - tri.v2, x - tri.v2), tri.Normal) < 0.0f)
            return false;
        outPoint = x;
        return true;
    }

    static bool SphereTriangleContact(const Vec3& centre, float radius, const Vec3& onTriangle, bool interior, const MeshTriangle& tri, MeshContact& out)
    {
        const Vec3 away   = centre - onTriangle;
        const float dist2 = Maths::Length2(away);
        if(dist2 >= radius * radius)
            return false;

        const float dist = Maths::Sqrt(dist2);
        out.Point        = onTriangle;
        out.Normal       = dist > 1e-5f ? away / dist : tri.Normal;
        out.Depth        = radius - dist;
        out.Face         = interior;
        return true;
    }

    static bool CapsuleTriangleContact(const Vec3& a, const Vec3& b, float radius, const MeshTriangle& tri, MeshContact& out)
    {
        // the core segment goes right through: push back out the way the centre came from
        Vec3 through;
        if(SegmentThroughTriangle(a, b, tri, through))
        {
            Vec3 normal = tri.Normal;
            if(Maths::Dot(normal, (a + b) * 0.5f - tri.v0) < 0.0f)
                normal = normal * -1.0f;
            const float behind = Maths::Max(-Maths::Dot(normal, a - tri.v0), -Maths::Dot(normal, b - tri.v0));
            out.Point          = through;
            out.Normal         = normal;
            out.Depth          = radius + Maths::Max(behind, 0.0f);
            out.Face           = true;
            return true;
        }

        // otherwise the nearest pair of points between the segment and the triangle
        bool interiorA, interiorB;
        const Vec3 nearA = ClosestPointOnTriangle(a, tri, interiorA);
        const Vec3 nearB = ClosestPointOnTriangle(b, tri, interiorB);

        Vec3 onSegment  = a, onTriangle = nearA;
        bool interior   = interiorA;
        float bestDist2 = Maths::Length2(a - nearA);
        const float dB  = Maths::Length2(b - nearB);
        if(dB < bestDist2)
        {
            bestDist2  = dB;
            onSegment  = b;
            onTriangle = nearB;
            interior   = interiorB;
        }

        const Vec3* corners[3] = { &tri.v0, &tri.v1, &tri.v2 };
        for(int e = 0; e < 3; e++)
        {
            Vec3 c1, c2;
            ClosestPointsOnSegments(a, b, *corners[e], *corners[(e + 1) % 3], c1, c2);
            const float d = Maths::Length2(c1 - c2);
            if(d < bestDist2 - 1e-8f)
            {
                bestDist2  = d;
                onSegment  = c1;
                onTriangle = c2;
                interior   = false;
            }
        }
        return SphereTriangleContact(onSegment, radius, onTriangle, interior, tri, out);
    }

    // Keeps the deepest few, one per place. An edge contact that lies in the plane of a face
    // contact is the seam between two triangles of one flat surface, not a real edge.
    static uint32_t SelectMeshContacts(MeshContact* all, uint32_t count, MeshContact* out)
    {
        for(uint32_t i = 0; i < count;)
        {
            bool seam = false;
            if(!all[i].Face)
            {
                for(uint32_t j = 0; j < count && !seam; j++)
                    seam = all[j].Face && Maths::Abs(Maths::Dot(all[j].Normal, all[i].Point - all[j].Point)) < 0.01f;
            }
            if(seam)
                all[i] = all[--count];
            else
                i++;
        }

        uint32_t kept = 0;
        while(kept < kMaxMeshContacts && count > 0)
        {
            uint32_t deepest = 0;
            for(uint32_t i = 1; i < count; i++)
                if(all[i].Depth > all[deepest].Depth)
                    deepest = i;

            const MeshContact c = all[deepest];
            all[deepest]        = all[--count];

            bool duplicate = false;
            for(uint32_t k = 0; k < kept && !duplicate; k++)
                duplicate = Maths::Dot(out[k].Normal, c.Normal) > 0.98f && Maths::Length2(out[k].Point - c.Point) < 0.05f * 0.05f;
            if(!duplicate)
                out[kept++] = c;
        }
        return kept;
    }

    static uint32_t CollectMeshContacts(RigidBody3D* meshObj, MeshCollisionShape* mesh, RigidBody3D* otherObj, CollisionShape* otherShape, const Mat4& toLocal, MeshContact* out)
    {
        static constexpr uint32_t kMaxTriangles = 256;
        static constexpr uint32_t kMaxFound     = 64;
        uint32_t triangles[kMaxTriangles];
        MeshContact found[kMaxFound];
        uint32_t foundCount = 0;

        const Mat4 otherToLocal = toLocal * otherObj->GetWorldSpaceTransform();

        switch(otherShape->GetType())
        {
        case CollisionShapeType::CollisionSphere:
        {
            const float radius = static_cast<SphereCollisionShape*>(otherShape)->GetRadius();
            const Vec3 centre  = Vec3(otherToLocal * Vec4(0.0f, 0.0f, 0.0f, 1.0f));
            const uint32_t n   = mesh->QueryAABB(centre - Vec3(radius), centre + Vec3(radius), triangles, kMaxTriangles);
            for(uint32_t i = 0; i < n && foundCount < kMaxFound; i++)
            {
                const MeshTriangle& tri = mesh->GetTriangle(triangles[i]);
                bool interior;
                const Vec3 nearest = ClosestPointOnTriangle(centre, tri, interior);
                if(SphereTriangleContact(centre, radius, nearest, interior, tri, found[foundCount]))
                    foundCount++;
            }
            break;
        }
        case CollisionShapeType::CollisionCapsule:
        {
            auto* capsule      = static_cast<CapsuleCollisionShape*>(otherShape);
            const float radius = capsule->GetRadius();
            const float halfH  = capsule->GetHeight() * 0.5f;
            const Vec3 a       = Vec3(otherToLocal * Vec4(0.0f, -halfH, 0.0f, 1.0f));
            const Vec3 b       = Vec3(otherToLocal * Vec4(0.0f, halfH, 0.0f, 1.0f));
            const Vec3 lo      = Vec3(Maths::Min(a.x, b.x), Maths::Min(a.y, b.y), Maths::Min(a.z, b.z)) - Vec3(radius);
            const Vec3 hi      = Vec3(Maths::Max(a.x, b.x), Maths::Max(a.y, b.y), Maths::Max(a.z, b.z)) + Vec3(radius);
            const uint32_t n   = mesh->QueryAABB(lo, hi, triangles, kMaxTriangles);
            for(uint32_t i = 0; i < n && foundCount < kMaxFound; i++)
            {
                if(CapsuleTriangleContact(a, b, radius, mesh->GetTriangle(triangles[i]), found[foundCount]))
                    foundCount++;
            }
            break;
        }
        default:
        {
            // Boxes and hulls: a corner has gone in when it is behind a triangle's plane and over
            // the triangle. Edges resting across a mesh edge are not caught.
            static constexpr uint32_t kMaxCorners = 32;
            Vec3 corners[kMaxCorners];
            uint32_t cornerCount = 0;

            if(otherShape->GetType() == CollisionShapeType::CollisionCuboid)
            {
                const Vec3 half = static_cast<CuboidCollisionShape*>(otherShape)->GetHalfDimensions();
                for(int i = 0; i < 8; i++)
                    corners[cornerCount++] = Vec3(otherToLocal * Vec4((i & 1) ? half.x : -half.x, (i & 2) ? half.y : -half.y, (i & 4) ? half.z : -half.z, 1.0f));
            }
            else if(Hull* hull = otherShape->GetHull())
            {
                const Mat4 hullToLocal = otherToLocal * otherShape->GetLocalTransform();
                for(size_t i = 0; i < hull->GetNumVertices() && cornerCount < kMaxCorners; i++)
                    corners[cornerCount++] = Vec3(hullToLocal * Vec4(hull->GetVertex((int)i).pos, 1.0f));
            }
            if(cornerCount == 0)
                break;

            const Vec3 centre = Vec3(otherToLocal * Vec4(0.0f, 0.0f, 0.0f, 1.0f));
            Vec3 lo = centre, hi = centre;
            float reach = FLT_MAX;
            for(uint32_t i = 0; i < cornerCount; i++)
            {
                lo    = Vec3(Maths::Min(lo.x, corners[i].x), Maths::Min(lo.y, corners[i].y), Maths::Min(lo.z, corners[i].z));
                hi    = Vec3(Maths::Max(hi.x, corners[i].x), Maths::Max(hi.y, corners[i].y), Maths::Max(hi.z, corners[i].z));
                reach = Maths::Min(reach, Maths::Length(corners[i] - centre));
            }
            // A centre this far behind a triangle is still a body that sank in from the front,
            // further back and it is on the other side of a thin wall.
            const float sunk = reach * 0.3f;

            const uint32_t n = mesh->QueryAABB(lo, hi, triangles, kMaxTriangles);
            for(uint32_t c = 0; c < cornerCount && foundCount < kMaxFound; c++)
            {
                MeshContact deepest;
                deepest.Depth = 0.0f;
                for(uint32_t i = 0; i < n; i++)
                {
                    const MeshTriangle& tri = mesh->GetTriangle(triangles[i]);
                    Vec3 normal             = tri.Normal;
                    if(Maths::Dot(normal, centre - tri.v0) < -sunk)
                        normal = normal * -1.0f;

                    const float depth = -Maths::Dot(normal, corners[c] - tri.v0);
                    if(depth <= deepest.Depth || depth > reach * 2.0f)
                        continue;

                    const Vec3 onPlane = corners[c] + normal * depth;
                    if(Maths::Dot(Maths::Cross(tri.v1 - tri.v0, onPlane - tri.v0), tri.Normal) < 0.0f
                        || Maths::Dot(Maths::Cross(tri.v2 - tri.v1, onPlane - tri.v1), tri.Normal) < 0.0f
                        || Maths::Dot(Maths::Cross(tri.v0 - tri.v2, onPlane - tri.v2), tri.Normal) < 0.0f)
                        continue;

                    deepest.Depth  = depth;
                    deepest.Normal = normal;
                    deepest.Point  = onPlane;
                    deepest.Face   = true;
                }
                if(deepest.Depth > 0.0f)
                    found[foundCount++] = deepest;
            }
            break;
        }
        }

        return SelectMeshContacts(found, foundCount, out);
    }

    bool CollisionDetection::CheckMeshCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        const bool meshIsA      = shape1->GetType() == CollisionShapeType::CollisionMesh;
        auto* mesh              = static_cast<MeshCollisionShape*>(meshIsA ? shape1 : shape2);
        RigidBody3D* meshObj    = meshIsA ? obj1 : obj2;
        RigidBody3D* otherObj   = meshIsA ? obj2 : obj1;
        CollisionShape* other   = meshIsA ? shape2 : shape1;
        MeshContactCache& cache = t_MeshContacts;

        cache.MeshBody      = meshObj;
        cache.OtherBody     = otherObj;
        cache.OtherPosition = otherObj->GetPosition();
        cache.Count         = 0;
        if(!mesh->IsBuilt())
            return false;

        const Mat4 meshToWorld = meshObj->GetWorldSpaceTransform() * mesh->GetLocalTransform();
        cache.Count            = CollectMeshContacts(meshObj, mesh, otherObj, other, Mat4::Inverse(meshToWorld), cache.Contacts);
        if(cache.Count == 0)
            return false;

        // to world space
        for(uint32_t i = 0; i < cache.Count; i++)
        {
            cache.Contacts[i].Point  = Vec3(meshToWorld * Vec4(cache.Contacts[i].Point, 1.0f));
            cache.Contacts[i].Normal = Vec3(meshToWorld * Vec4(cache.Contacts[i].Normal, 0.0f));
        }

        if(out_coldata)
        {
            const MeshContact& deepest = cache.Contacts[0];
            out_coldata->normal        = meshIsA ? deepest.Normal : deepest.Normal * -1.0f;
            out_coldata->penetration   = -deepest.Depth; // negative = overlapping
            out_coldata->pointOnPlane  = deepest.Point;
        }
        return true;
    }

    bool CollisionDetection::CheckCollisionAxis(const Vec3& axis, RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        Vec3 min1, min2, max1, max2;

        shape1->GetMinMaxVertexOnAxis(obj1, axis, &min1, &max1);
        shape2->GetMinMaxVertexOnAxis(obj2, axis, &min2, &max2);

        float minCorrelation1 = Maths::Dot(axis, min1);
        float maxCorrelation1 = Maths::Dot(axis, max1);
        float minCorrelation2 = Maths::Dot(axis, min2);
        float maxCorrelation2 = Maths::Dot(axis, max2);

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

    static bool BuildCapsuleSideContacts(RigidBody3D* capsuleObj, CapsuleCollisionShape* capsule,
                                         CollisionShape* otherShape, RigidBody3D* otherObj,
                                         const CollisionData& coldata, bool capsuleIsA, Manifold* manifold)
    {
        ReferencePolygon face;
        const Vec3 otherAxis = capsuleIsA ? -coldata.normal : coldata.normal;
        otherShape->GetIncidentReferencePolygon(otherObj, otherAxis, face);
        if(face.FaceCount < 3) // sphere/capsule opponent — no face to clip to
            return false;

        // Capsule inner segment (local +/-Y) in world space.
        const float halfH  = capsule->GetHeight() * 0.5f;
        const float radius = capsule->GetRadius();
        const Mat4& ws     = capsuleObj->GetWorldSpaceTransform();
        const Vec3 s0      = ws * Vec4(0.0f, -halfH, 0.0f, 1.0f);
        const Vec3 s1      = ws * Vec4(0.0f, halfH, 0.0f, 1.0f);

        const Vec3 seg     = s1 - s0;
        const float segLen = Maths::Length(seg);
        if(segLen < Maths::M_EPSILON)
            return false;
        const Vec3 segDir = seg / segLen;

        // End-on: capsule axis ~parallel to the normal, one cap point is right.
        if(Maths::Abs(Maths::Dot(segDir, coldata.normal)) > 0.965f) // ~15 deg
            return false;

        float tEnter = 0.0f, tExit = 1.0f;
        for(uint32_t i = 1; i < face.PlaneCount; i++)
        {
            const Plane& pl  = face.AdjacentPlanes[i];
            const float d0   = pl.Distance(s0);
            const float d1   = pl.Distance(s1);
            const float diff = d1 - d0;
            if(Maths::Abs(diff) < Maths::M_EPSILON)
            {
                if(d0 < 0.0f)
                    return false; // segment entirely outside this side plane
                continue;
            }
            const float t = -d0 / diff; // crossing where Distance == 0
            if(diff > 0.0f)
                tEnter = Maths::Max(tEnter, t);
            else
                tExit = Maths::Min(tExit, t);
        }
        if(tEnter > tExit)
            return false; // shaft doesn't overlap the face laterally

        const Plane facePlane(face.Normal, -Maths::Dot(face.Normal, face.Faces[0]));
        const Vec3 surfDir = capsuleIsA ? coldata.normal : -coldata.normal; // capsule -> other

        const float ts[2]      = { tEnter, tExit };
        const bool singlePoint = (Maths::Abs(tExit - tEnter) * segLen < 0.01f);
        int added              = 0;
        for(int i = 0; i < 2; i++)
        {
            const Vec3 segPoint = s0 + seg * ts[i];
            const Vec3 surface  = segPoint + surfDir * radius;

            float pen = facePlane.Distance(surface); // negative = penetrating
            if(pen > 0.02f)                          // this end isn't touching
                continue;
            pen = Maths::Min(pen, 0.0f);

            if(capsuleIsA)
                manifold->AddContact(surface, surface - coldata.normal * pen, coldata.normal, pen);
            else
                manifold->AddContact(surface + coldata.normal * pen, surface, coldata.normal, pen);
            added++;

            if(singlePoint)
                break;
        }
        return added > 0;
    }

    bool CollisionDetection::BuildCollisionManifold(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData& coldata, Manifold* manifold)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        if(!manifold)
            return false;

        // Terrain: use collision data directly (heightfield can't produce reference polygons)
        if(shape1->GetType() == CollisionShapeType::CollisionTerrain)
        {
            Vec3 globalOnA = coldata.pointOnPlane;
            Vec3 globalOnB = coldata.pointOnPlane + coldata.normal * coldata.penetration;
            manifold->AddContact(globalOnA, globalOnB, coldata.normal, coldata.penetration);
            return true;
        }
        if(shape2->GetType() == CollisionShapeType::CollisionTerrain)
        {
            Vec3 globalOnA = coldata.pointOnPlane - coldata.normal * coldata.penetration;
            Vec3 globalOnB = coldata.pointOnPlane;
            manifold->AddContact(globalOnA, globalOnB, coldata.normal, coldata.penetration);
            return true;
        }

        // Mesh: one contact per surface touched (floor and wall at once), found by the check just before
        if(shape1->GetType() == CollisionShapeType::CollisionMesh || shape2->GetType() == CollisionShapeType::CollisionMesh)
        {
            const bool meshIsA      = shape1->GetType() == CollisionShapeType::CollisionMesh;
            RigidBody3D* meshObj    = meshIsA ? obj1 : obj2;
            RigidBody3D* otherObj   = meshIsA ? obj2 : obj1;
            MeshContactCache& cache = t_MeshContacts;
            if(cache.MeshBody != meshObj || cache.OtherBody != otherObj || cache.OtherPosition != otherObj->GetPosition())
            {
                CollisionData ignored;
                if(!CheckMeshCollision(obj1, obj2, shape1, shape2, &ignored))
                    return false;
            }

            for(uint32_t i = 0; i < cache.Count; i++)
            {
                const MeshContact& c = cache.Contacts[i];
                const Vec3 onOther   = c.Point - c.Normal * c.Depth;
                if(meshIsA)
                    manifold->AddContact(c.Point, onOther, c.Normal, -c.Depth);
                else
                    manifold->AddContact(onOther, c.Point, c.Normal * -1.0f, -c.Depth);
            }
            return cache.Count > 0;
        }

        {
            const bool cap1 = shape1->GetType() == CollisionShapeType::CollisionCapsule;
            const bool cap2 = shape2->GetType() == CollisionShapeType::CollisionCapsule;
            if(cap1 != cap2)
            {
                if(cap1 && BuildCapsuleSideContacts(obj1, static_cast<CapsuleCollisionShape*>(shape1), shape2, obj2, coldata, true, manifold))
                    return true;
                if(cap2 && BuildCapsuleSideContacts(obj2, static_cast<CapsuleCollisionShape*>(shape2), shape1, obj1, coldata, false, manifold))
                    return true;
            }
        }

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
            Vec3* incPolygon;
            Vec3* refPolygon;
            int incPolygonCount;
            int refPolygonCount;
            Plane* refAdjPlanes;
            int refAdjPlanesCount;
            Plane refPlane;

            if(Maths::Abs(Maths::Dot(coldata.normal, poly1.Normal)) > Maths::Abs(Maths::Dot(coldata.normal, poly2.Normal)))
            {
                float planeDist = -(Maths::Dot(poly1.Faces[0], -poly1.Normal));
                refPlane        = Plane(-poly1.Normal, planeDist);

                refAdjPlanes      = poly1.AdjacentPlanes;
                refAdjPlanesCount = poly1.PlaneCount;
                incPolygon        = poly2.Faces;
                refPolygon        = poly1.Faces;
                incPolygonCount   = poly2.FaceCount;
                refPolygonCount   = poly1.FaceCount;

                flipped = false;
            }
            else
            {
                float planeDist = -(Maths::Dot(poly2.Faces[0], -poly2.Normal));
                refPlane        = Plane(-poly2.Normal, planeDist);

                refAdjPlanes      = poly2.AdjacentPlanes;
                refAdjPlanesCount = poly2.PlaneCount;
                incPolygon        = poly1.Faces;
                refPolygon        = poly2.Faces;
                incPolygonCount   = poly1.FaceCount;
                refPolygonCount   = poly2.FaceCount;

                flipped = true;
            }

            // Determine largest penetration
            float penetrationOffset = -FLT_MAX;
            for(auto it = 0; it != refPolygonCount; ++it)
            {
                float pOffset = Maths::Dot(refPolygon[it], coldata.normal);
                if(pOffset > penetrationOffset)
                    penetrationOffset = pOffset;
            }

            ArenaTemp scratch = ScratchBegin(nullptr, 0);
            TDArray<Vec3> incPolygonList(scratch.arena);
            incPolygonList.Resize(incPolygonCount);
            MemoryCopy(incPolygonList.Data(), incPolygon, sizeof(Vec3) * incPolygonCount);

            SutherlandHodgesonClipping(scratch.arena, incPolygonList, refAdjPlanesCount, refAdjPlanes, &incPolygonList, false);
            SutherlandHodgesonClipping(scratch.arena, incPolygonList, 1, &refPlane, &incPolygonList, true);

            for(auto it = incPolygonList.begin(); it != incPolygonList.end(); ++it)
            {
                float contact_penetration;
                Vec3 globalOnA, globalOnB;

                if(flipped)
                {
                    contact_penetration = -Maths::Dot(*it, coldata.normal)
                        + penetrationOffset; // +(Maths::Dot(coldata.normal, poly2.Faces[0]));

                    globalOnA = *it + (coldata.normal * contact_penetration);
                    globalOnB = *it;
                }
                else
                {
                    contact_penetration = Maths::Dot(*it, coldata.normal) - penetrationOffset; // Maths::Dot(coldata.normal, poly1.Faces[0]);

                    globalOnA = *it;
                    globalOnB = *it - (coldata.normal * contact_penetration);
                }

                // if(globalOnB.z > 30.0f)
                //     LINFO("Large value in manifold creation %.2f,%.2f,%.2f", globalOnB.x, globalOnB.y, globalOnB.z);

                if(contact_penetration < 0.0f)
                    manifold->AddContact(globalOnA, globalOnB, coldata.normal, contact_penetration);
            }
            ScratchEnd(scratch);
        }
        return true;
    }

    Vec3 CollisionDetection::GetClosestPointOnEdges(const Vec3& target, const TDArray<CollisionEdge>& edges)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        Vec3 closest_point      = Vec3(0.0f);
        Vec3 temp_closest_point = Vec3(0.0f);
        float closest_distsq    = FLT_MAX;

        for(const CollisionEdge& edge : edges)
        {
            Vec3 a_t = target - edge.posA;
            Vec3 a_b = edge.posB - edge.posA;

            float magnitudeAB = Maths::Dot(a_b, a_b);      // Magnitude of AB vector (it's length squared)
            float ABAPproduct = Maths::Dot(a_t, a_b);      // The DOT product of a_to_t and a_to_b
            float distance    = ABAPproduct / magnitudeAB; // The Normalised "distance" from a to your closest point

            if(distance < 0.0f) // Clamp returned point to be on the line, e.g if the closest point is beyond the AB return either A or B as closest points
                temp_closest_point = edge.posA;

            else if(distance > 1)
                temp_closest_point = edge.posB;
            else
                temp_closest_point = edge.posA + a_b * distance;

            Vec3 c_t          = target - temp_closest_point;
            float temp_distsq = Maths::Dot(c_t, c_t);

            if(temp_distsq < closest_distsq)
            {
                closest_distsq = temp_distsq;
                closest_point  = temp_closest_point;
            }
        }

        return closest_point;
    }

    Vec3 CollisionDetection::PlaneEdgeIntersection(const Plane& plane, const Vec3& start, const Vec3& end) const
    {
        Vec3 ab = end - start;

        float ab_p = Maths::Dot(plane.Normal(), ab);

        if(Maths::Abs(ab_p) > Maths::M_EPSILON)
        {
            Vec3 p_co = plane.Normal() * (-plane.Distance(Vec3(0.0f)));

            Vec3 w    = start - p_co;
            float fac = -(Maths::Dot(plane.Normal(), w)) / ab_p;
            ab        = ab * fac;

            return start + ab;
        }

        return start;
    }

    void CollisionDetection::SutherlandHodgesonClipping(Arena* arena, const TDArray<Vec3>& input_polygon, int num_clip_planes, const Plane* clip_planes, TDArray<Vec3>* out_polygon, bool removePoints) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        if(!out_polygon)
            return;

        // Create temporary list of vertices
        // - We will keep ping-pong'ing between
        //   the two lists updating them as we go.
        TDArray<Vec3> ppPolygon1(arena), ppPolygon2(arena);
        TDArray<Vec3>*input = &ppPolygon1, *output = &ppPolygon2;

        *output = input_polygon;

        // Iterate over each clip_plane provided
        for(int i = 0; i < num_clip_planes; ++i)
        {
            // If we every single point on our shape has already been removed, just exit
            if(output->Empty())
                break;

            const Plane& plane = clip_planes[i];

            // Swap input/output polygons, and clear output list for us to generate afresh
            Swap(input, output);
            output->Clear();

            // Loop through each edge of the polygon (see line_loop from gfx) and clips
            // that edge against the plane.
            Vec3 startPoint = input->Back();
            for(const Vec3& endPoint : *input)
            {
                bool startInPlane = plane.IsPointOnPlane(startPoint);
                bool endInPlane   = plane.IsPointOnPlane(endPoint);

                // If it's the final pass, just remove all points outside the reference
                // plane
                if(removePoints)
                {
                    if(endInPlane)
                        output->EmplaceBack(endPoint);
                }
                else
                {
                    // if entire edge is within the clipping plane, keep it as it is
                    if(startInPlane && endInPlane)
                        output->EmplaceBack(endPoint);

                    // if edge interesects the clipping plane, cut the edge along clip plane
                    else if(startInPlane && !endInPlane)
                    {
                        output->EmplaceBack(PlaneEdgeIntersection(plane, startPoint, endPoint));
                    }
                    else if(!startInPlane && endInPlane)
                    {
                        output->EmplaceBack(PlaneEdgeIntersection(plane, endPoint, startPoint));
                        output->EmplaceBack(endPoint);
                    }
                }
                //..otherwise the edge is entirely outside the clipping plane and should
                // be removed

                startPoint = endPoint;
            }
        }

        *out_polygon = *output;
    }

    bool CollisionDetection::CheckSphereOverlap(const Vec3& pos1, float radius1, const Vec3& pos2, float radius2)
    {
        return Maths::Distance2(pos2, pos1) <= Maths::Squared(radius1 + radius2);
    }

    bool CollisionDetection::CheckAABBOverlap(const Vec3& pos1, const Vec3& halfHidth1, const Vec3& pos2, const Vec3& halfHidth2)
    {
        if(abs(pos1.x - pos2.x) >= (halfHidth1.x + halfHidth2.x))
            return false;
        if(abs(pos1.y - pos2.y) >= (halfHidth1.y + halfHidth2.y))
            return false;
        if(abs(pos1.z - pos2.z) >= (halfHidth1.z + halfHidth2.z))
            return false;
        return true;
    }

    bool CollisionDetection::CheckAABBSphereOverlap(const Vec3& center, const Vec3& halfVol, const Vec3& spherePos, float sphereRad)
    {
        const Vec3 minVol = center - halfVol;
        const Vec3 maxVol = center + halfVol;
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

    bool CollisionDetection::CheckSphereInsideAABB(const Vec3& spherePos, float sphereRadius, const Vec3& AABBCenter, const Vec3& AABBHalfVol)
    {
        // min check
        Vec3 minPoint = AABBCenter - AABBHalfVol;
        if(minPoint.x > spherePos.x - sphereRadius)
            return false;
        if(minPoint.y > spherePos.y - sphereRadius)
            return false;
        if(minPoint.z > spherePos.z - sphereRadius)
            return false;
        // max check
        Vec3 maxPoint = AABBCenter + AABBHalfVol;
        if(maxPoint.x < spherePos.x + sphereRadius)
            return false;
        if(maxPoint.y < spherePos.y + sphereRadius)
            return false;
        if(maxPoint.z < spherePos.z + sphereRadius)
            return false;

        return true;
    }

    bool CollisionDetection::CheckAABBInsideAABB(const Vec3& AABBInsideCenter, const Vec3& AABBInsideHalfVol, const Vec3& AABBCenter, const Vec3& AABBHalfVol)
    {
        // min check
        Vec3 minPoint       = AABBCenter - AABBHalfVol;
        Vec3 minInsidePoint = AABBInsideCenter - AABBInsideHalfVol;
        if(minPoint.x > minInsidePoint.x)
            return false;
        if(minPoint.y > minInsidePoint.y)
            return false;
        if(minPoint.z > minInsidePoint.z)
            return false;
        // max check
        Vec3 maxPoint       = AABBCenter + AABBHalfVol;
        Vec3 maxInsidePoint = AABBInsideCenter + AABBInsideHalfVol;
        if(maxPoint.x < maxInsidePoint.x)
            return false;
        if(maxPoint.y < maxInsidePoint.y)
            return false;
        if(maxPoint.z < maxInsidePoint.z)
            return false;

        return true;
    }
}
