#include "Precompiled.h"
#include "CollisionDetection.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/HullCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CapsuleCollisionShape.h"
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
    }

    bool CollisionDetection::CheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        ASSERT(((shape1->GetType() | shape2->GetType()) < m_MaxSize), "Invalid collision func %i, %i, %i, %i", (int)shape1->GetType(), (int)shape2->GetType(), (int)shape2->GetType() | (int)shape2->GetType(), m_MaxSize);
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
        colData.pointOnPlane = obj1->GetPosition() + axis * 0.5f;

        if(out_coldata)
            *out_coldata = colData;

        return true;
    }

    void AddPossibleCollisionAxis(Vec3& axis, Vec3* possibleCollisionAxes, uint32_t& possibleCollisionAxesCount)
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

        static const int MAX_COLLISION_AXES = 100;
        static Vec3 possibleCollisionAxes[MAX_COLLISION_AXES];

        uint32_t possibleCollisionAxesCount = 0;
        for(const Vec3& axis : shapeCollisionAxes)
        {
            possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
        }

        AddPossibleCollisionAxis(p_t, possibleCollisionAxes, possibleCollisionAxesCount);

        for(uint32_t i = 0; i < possibleCollisionAxesCount; i++)
        {
            const Vec3& axis = possibleCollisionAxes[i];
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

        static const int MAX_COLLISION_AXES = 100;
        static Vec3 possibleCollisionAxes[MAX_COLLISION_AXES];

        uint32_t possibleCollisionAxesCount = 0;
        for(const Vec3& axis : shape1CollisionAxes)
        {
            possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
        }

        for(const Vec3& axis : shape2PossibleCollisionAxes)
        {
            possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
        }

        TDArray<CollisionEdge>& shape1_edges = shape1->GetEdges(obj1);
        TDArray<CollisionEdge>& shape2_edges = shape2->GetEdges(obj2);

        for(uint32_t i = 0; i < possibleCollisionAxesCount; i++)
        {
            const Vec3& axis = possibleCollisionAxes[i];
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

        Vec3 d     = p2 - p1;
        float dist = Maths::Length(d);

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
        bool areParallel = Maths::AreVectorsParallel(seg1, seg2);

        if(areParallel)
        {
            float segmentsPerpendicularDistance = PointToLineDistance(capsule1SegA, capsule1SegB, capsule2SegA);
            if(segmentsPerpendicularDistance > radiusSum)
                return false;

            float d1 = Maths::Dot(seg1, Vec3(capsule1SegA));
            float d2 = -Maths::Dot(seg1, Vec3(capsule1SegB));

            float t1 = PlaneSegmentIntersection(capsule2SegB, capsule2SegA, d1, seg1);
            float t2 = PlaneSegmentIntersection(capsule2SegA, capsule2SegB, d2, -seg1);

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
                // Vec3 normalWorld = obj2->GetOrientation() * normalCapsule2SpaceNormalized;

                float correlation1 = Maths::Dot(normalWorld, Vec3(contactPointACapsule1Local));
                float correlation2 = Maths::Dot(normalWorld, Vec3(contactPointBCapsule1Local));

                static bool flipNormal = false;
                // flipNormal = !flipNormal;
                // if(correlation1 <= correlation2)
                //  if(Maths::Length(normalWorld, p2 - p1) < 0.0f)
                //  {
                //      normalWorld = -normalWorld;
                //  }

                // Flip Normal if needed
                if(Maths::Dot(normalWorld, p2 - p1) < 0.0f)
                // if(flipNormal)
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

                // const Vec3 normalWorld = obj2->GetOrientation() * closestPointsSeg1ToSeg2;
                Vec3 normalWorld = Mat4(obj2->GetOrientation()) * Vec4(closestPointsSeg1ToSeg2, 1.0f);

                float penetrationDepth = radiusSum - closestPointsDistance;

                // Create the contact info object

                float correlation1 = Maths::Dot(normalWorld, Vec3(contactPointCapsule1Local));
                float correlation2 = Maths::Dot(normalWorld, Vec3(contactPointCapsule2Local));

                // if(correlation1 <= correlation2)
                if(Maths::Dot(normalWorld, p2 - p1) < 0.0f)
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

                    // const Vec3 normalWorld = obj2->GetOrientation() * Vec4(normalCapsuleSpace2, 1.0f);
                    Vec3 normalWorld = Mat4(obj2->GetOrientation()) * Vec4(normalCapsuleSpace2, 1.0f);

                    float correlation1 = Maths::Dot(normalWorld, Vec3(contactPointCapsule1Local));
                    float correlation2 = Maths::Dot(normalWorld, Vec3(contactPointCapsule2Local));

                    // if(correlation1 <= correlation2)
                    if(Maths::Dot(normalWorld, p2 - p1) < 0.0f)
                    {
                        normalWorld = -normalWorld;
                    }
                    // Create the contact info object
                    best_colData.normal       = normalWorld.Normalised();
                    best_colData.penetration  = radiusSum;
                    best_colData.pointOnPlane = obj1->GetWorldSpaceTransform() * Vec4(contactPointCapsule1Local, 1.0f);

                    if(out_coldata)
                        *out_coldata = best_colData;

                    return true;
                }
                else
                {
                    Vec3 normalCapsuleSpace2 = Maths::Cross(seg1, seg2);
                    normalCapsuleSpace2.Normalise();

                    const Vec3 contactPointCapsule1Local = Mat4::Inverse(capsule1ToCapsule2SpaceTransform) * (closestPointCapsule1Seg + normalCapsuleSpace2 * capsule1Radius);
                    const Vec3 contactPointCapsule2Local = closestPointCapsule2Seg - normalCapsuleSpace2 * capsule2Radius;

                    // const Vec3 normalWorld = obj2->GetOrientation() * Vec4(normalCapsuleSpace2, 1.0f);
                    Vec3 normalWorld = Mat4(obj2->GetOrientation()) * Vec4(normalCapsuleSpace2, 1.0f);

                    float correlation1 = Maths::Dot(normalWorld, Vec3(contactPointCapsule1Local));
                    float correlation2 = Maths::Dot(normalWorld, Vec3(contactPointCapsule2Local));

                    // if(correlation1 <= correlation2)
                    if(Maths::Dot(normalWorld, p2 - p1) < 0.0f)
                    {
                        normalWorld = -normalWorld;
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
            Vec3 contactPointSphereLocal;
            Vec3 contactPointCapsuleLocal;

            // If the sphere center is not on the capsule inner segment
            if(sphereSegmentDistanceSquare > Maths::M_EPSILON)
            {
                float sphereSegmentDistance = Maths::Sqrt(sphereSegmentDistanceSquare);
                sphereCenterToSegment /= sphereSegmentDistance;

                contactPointSphereLocal  = Mat4::Inverse(sphereToCapsuleSpaceTransform) * Vec4(sphereToCapsuleSpacePos + sphereCenterToSegment * sphereRadius, 1.0f);
                contactPointCapsuleLocal = closestPointOnSegment - sphereCenterToSegment * capsuleRadius;

                normalWorld = Mat4(capsuleObj->GetOrientation()) * Vec4(sphereCenterToSegment, 1.0f);

                penetrationDepth = sumRadius - sphereSegmentDistance;

                if(obj1 != sphereObj)
                    normalWorld = -normalWorld;
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

                // Compute the two local contact points
                contactPointSphereLocal  = Mat4::Inverse(sphereToCapsuleSpaceTransform) * Vec4(sphereToCapsuleSpacePos + normalCapsuleSpace * sphereRadius, 1.0f);
                contactPointCapsuleLocal = sphereToCapsuleSpacePos - normalCapsuleSpace * capsuleRadius;
            }

            if(penetrationDepth <= 0.0f)
                return false;

            colData.normal       = normalWorld.Normalised();
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

        static const int MAX_COLLISION_AXES = 100;
        static Vec3 possibleCollisionAxes[MAX_COLLISION_AXES];

        uint32_t possibleCollisionAxesCount = 0;
        for(const Vec3& axis : shapeCollisionAxes)
        {
            possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
        }

        AddPossibleCollisionAxis(p_t, possibleCollisionAxes, possibleCollisionAxesCount);

        Vec3 capsulePos = capsuleObj->GetPosition();
        Vec4 forward    = Vec4(0.0f, 0.0f, 1.0f, 0.0f);
        Vec3 capsuleDir = capsuleObj->GetWorldSpaceTransform() * forward;

        float capsuleRadius = capsuleShape->GetRadius();
        float capsuleHeight = capsuleShape->GetHeight();

        float capsuleTop    = capsulePos.y + capsuleHeight * 0.5f;
        float capsuleBottom = capsulePos.y - capsuleHeight * 0.5f;

        for(uint32_t i = 0; i < possibleCollisionAxesCount; i++)
        {
            const Vec3& axis = possibleCollisionAxes[i];
            if(!CheckCollisionAxis(axis, obj1, obj2, shape1, shape2, &cur_colData))
                return false;

            if(cur_colData.penetration >= best_colData.penetration)
                best_colData = cur_colData;
        }

        if(Maths::Dot(best_colData.normal, capsuleDir) < 0.0f)
            best_colData.normal = -best_colData.normal;

        if(out_coldata)
            *out_coldata = best_colData;

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

    bool CollisionDetection::BuildCollisionManifold(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData& coldata, Manifold* manifold)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
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
