#include "Precompiled.h"
#include "CollisionDetection.h"

#include "SphereCollisionShape.h"

namespace Lumos
{
    CollisionDetection::CollisionDetection()
    {
        //max actual size
        m_MaxSize = CollisionShapeTypeMax | (CollisionShapeTypeMax >> 1);
        LUMOS_LOG_INFO("Max Collision Size {0}", m_MaxSize);
        m_CollisionCheckFunctions = new CollisionCheckFunc[m_MaxSize];
        std::fill(m_CollisionCheckFunctions, m_CollisionCheckFunctions + m_MaxSize, &CollisionDetection::CheckPolyhedronCollision);

        m_CollisionCheckFunctions[CollisionSphere] = &CollisionDetection::CheckSphereCollision;
        m_CollisionCheckFunctions[CollisionCuboid] = &CollisionDetection::CheckPolyhedronCollision;
        m_CollisionCheckFunctions[CollisionPyramid] = &CollisionDetection::CheckPolyhedronCollision;
        m_CollisionCheckFunctions[CollisionHull] = &CollisionDetection::CheckPolyhedronCollision;

        m_CollisionCheckFunctions[CollisionSphere | CollisionCuboid] = &CollisionDetection::CheckPolyhedronSphereCollision;
        m_CollisionCheckFunctions[CollisionSphere | CollisionPyramid] = &CollisionDetection::CheckPolyhedronSphereCollision;
        m_CollisionCheckFunctions[CollisionSphere | CollisionHull] = &CollisionDetection::CheckPolyhedronSphereCollision;
    }

    bool CollisionDetection::CheckCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION();
        if((shape1->GetType() | shape2->GetType()) >= m_MaxSize)
            LUMOS_LOG_INFO("Invalid collision func {0}, {1}, {2}, {3}", shape1->GetType(), shape1->GetType(), shape2->GetType() | shape2->GetType(), m_MaxSize);
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
        CollisionData colData;
        Maths::Vector3 axis = obj2->GetPosition() - obj1->GetPosition();
        axis.Normalise();
        if(!CheckCollisionAxis(axis, obj1, obj2, shape1, shape2, &colData))
            return false;

        if(out_coldata)
            *out_coldata = colData;

        return true;
    }

    void AddPossibleCollisionAxis(Maths::Vector3& axis, std::vector<Maths::Vector3>* possible_collision_axes)
    {
        LUMOS_PROFILE_FUNCTION();
        const float epsilon = 0.0001f;

        if(axis.LengthSquared() < epsilon)
            return;

        axis.Normalise();

        float value = (1.0f - epsilon);
        auto& axes = *possible_collision_axes;

        for(const Maths::Vector3& p_axis : axes)
        {
            if(abs(Maths::Vector3::Dot(axis, p_axis)) >= value)
                return;
        }

        possible_collision_axes->push_back(axis);
    }

    bool CollisionDetection::CheckPolyhedronSphereCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION();
        CollisionShape* complexShape;
        RigidBody3D* complexObj;
        RigidBody3D* sphereObj;

        if(obj1->GetCollisionShape()->GetType() == CollisionShapeType::CollisionSphere)
        {
            sphereObj = obj1;
            complexShape = shape2;
            complexObj = obj2;
        }
        else
        {
            sphereObj = obj2;
            complexShape = shape1;
            complexObj = obj1;
        }

        CollisionData cur_colData;
        CollisionData best_colData;
        best_colData.penetration = -FLT_MAX;

        std::vector<Maths::Vector3> possibleCollisionAxes = complexShape->GetCollisionAxes(complexObj);
        std::vector<CollisionEdge>& complex_shape_edges = complexShape->GetEdges(complexObj);

        Maths::Vector3 p = GetClosestPointOnEdges(sphereObj->GetPosition(), complex_shape_edges);
        Maths::Vector3 p_t = sphereObj->GetPosition() - p;
        p_t.Normalise();
        AddPossibleCollisionAxis(p_t, &possibleCollisionAxes);

        for(const Maths::Vector3& axis : possibleCollisionAxes)
        {
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

        std::vector<Maths::Vector3> possibleCollisionAxes = shape1->GetCollisionAxes(obj1);
        std::vector<Maths::Vector3>& tempPossibleCollisionAxes = shape2->GetCollisionAxes(obj2);

        for(Maths::Vector3& temp : tempPossibleCollisionAxes)
            AddPossibleCollisionAxis(temp, &possibleCollisionAxes);

        std::vector<CollisionEdge>& shape1_edges = shape1->GetEdges(obj1);
        std::vector<CollisionEdge>& shape2_edges = shape2->GetEdges(obj2);

        for(const CollisionEdge& edge1 : shape1_edges)
        {
            for(const CollisionEdge& edge2 : shape2_edges)
            {
                Maths::Vector3 e1 = edge1.posB - edge1.posA;
                Maths::Vector3 e2 = edge2.posB - edge2.posA;
                e1.Normalise();
                e2.Normalise();

                Maths::Vector3 temp = e1.CrossProduct(e2);
                AddPossibleCollisionAxis(temp, &possibleCollisionAxes);
            }
        }

        for(const Maths::Vector3& axis : possibleCollisionAxes)
        {
            if(!CheckCollisionAxis(axis, obj1, obj2, shape1, shape2, &cur_colData))
                return false;

            if(cur_colData.penetration >= best_colData.penetration)
                best_colData = cur_colData;
        }

        if(out_coldata)
            *out_coldata = best_colData;

        return true;
    }

    bool CollisionDetection::CheckCollisionAxis(const Maths::Vector3& axis, RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
    {
        LUMOS_PROFILE_FUNCTION();
        Maths::Vector3 min1, min2, max1, max2;

        shape1->GetMinMaxVertexOnAxis(obj1, axis, &min1, &max1);
        shape2->GetMinMaxVertexOnAxis(obj2, axis, &min2, &max2);

        float minCorrelation1 = axis.DotProduct(min1);
        float maxCorrelation1 = axis.DotProduct(max1);
        float minCorrelation2 = axis.DotProduct(min2);
        float maxCorrelation2 = axis.DotProduct(max2);

        if(minCorrelation1 <= minCorrelation2 && maxCorrelation1 >= minCorrelation2)
        {
            if(out_coldata != nullptr)
            {
                out_coldata->normal = axis;
                out_coldata->penetration = minCorrelation2 - maxCorrelation1;
                out_coldata->pointOnPlane = max1 + out_coldata->normal * out_coldata->penetration;
            }
            return true;
        }

        if(minCorrelation2 <= minCorrelation1 && maxCorrelation2 > minCorrelation1)
        {
            if(out_coldata != nullptr)
            {
                out_coldata->normal = -axis;
                out_coldata->penetration = minCorrelation1 - maxCorrelation2;
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
            Maths::Vector3* incPolygon;
            int incPolygonCount;
            Maths::Plane* refAdjPlanes;
            int refAdjPlanesCount;
            Maths::Plane refPlane;

            if(fabs(coldata.normal.DotProduct(poly1.Normal)) > fabs(coldata.normal.DotProduct(poly2.Normal)))
            {
                float planeDist = -(poly1.Faces[0].DotProduct(-poly1.Normal));
                refPlane = Maths::Plane(-poly1.Normal, planeDist);

                refAdjPlanes = poly1.AdjacentPlanes;
                refAdjPlanesCount = poly1.PlaneCount;
                incPolygon = poly2.Faces;
                incPolygonCount = poly2.FaceCount;

                flipped = false;
            }
            else
            {
                float planeDist = -(poly2.Faces[0].DotProduct(-poly2.Normal));
                refPlane = Maths::Plane(-poly2.Normal, planeDist);

                refAdjPlanes = poly2.AdjacentPlanes;
                refAdjPlanesCount = poly2.PlaneCount;
                incPolygon = poly1.Faces;
                incPolygonCount = poly1.FaceCount;

                flipped = true;
            }

            SutherlandHodgesonClipping(incPolygon, incPolygonCount, refAdjPlanesCount, refAdjPlanes, incPolygon, incPolygonCount, false);
            SutherlandHodgesonClipping(incPolygon, incPolygonCount, 1, &refPlane, incPolygon, incPolygonCount, true);

            for(int i = 0; i < incPolygonCount; i++)
            {
                auto& endPoint = incPolygon[i];
                float contact_penetration;
                Maths::Vector3 globalOnA, globalOnB;

                if(flipped)
                {
                    contact_penetration = -(endPoint.DotProduct(coldata.normal)
                        - (coldata.normal.DotProduct(poly2.Faces[0])));

                    globalOnA = endPoint + coldata.normal * contact_penetration;
                    globalOnB = endPoint;
                }
                else
                {
                    contact_penetration = endPoint.DotProduct(coldata.normal) - coldata.normal.DotProduct(poly1.Faces[0]);

                    globalOnA = endPoint;
                    globalOnB = endPoint - coldata.normal * contact_penetration;
                }

                //Thread safe here
                manifold->AddContact(globalOnA, globalOnB, coldata.normal, contact_penetration);
            }
        }
        return true;
    }

    Maths::Vector3 CollisionDetection::GetClosestPointOnEdges(const Maths::Vector3& target, const std::vector<CollisionEdge>& edges)
    {
        LUMOS_PROFILE_FUNCTION();
        Maths::Vector3 closest_point, temp_closest_point;
        float closest_distsq = FLT_MAX;

        for(const CollisionEdge& edge : edges)
        {
            Maths::Vector3 a_t = target - edge.posA;
            Maths::Vector3 a_b = edge.posB - edge.posA;

            float magnitudeAB = a_b.DotProduct(a_b); //Magnitude of AB vector (it's length squared)
            float ABAPproduct = a_t.DotProduct(a_b); //The DOT product of a_to_t and a_to_b
            float distance = ABAPproduct / magnitudeAB; //The Normalised "distance" from a to your closest point

            if(distance < 0.0f) //Clamp returned point to be on the line, e.g if the closest point is beyond the AB return either A or B as closest points
                temp_closest_point = edge.posA;

            else if(distance > 1)
                temp_closest_point = edge.posB;
            else
                temp_closest_point = edge.posA + a_b * distance;

            Maths::Vector3 c_t = target - temp_closest_point;
            float temp_distsq = c_t.DotProduct(c_t);

            if(temp_distsq < closest_distsq)
            {
                closest_distsq = temp_distsq;
                closest_point = temp_closest_point;
            }
        }

        return closest_point;
    }

    Maths::Vector3 CollisionDetection::PlaneEdgeIntersection(const Maths::Plane& plane, const Maths::Vector3& start, const Maths::Vector3& end) const
    {
        Maths::Vector3 ab = end - start;

        float ab_p = plane.normal_.DotProduct(ab);

        if(fabs(ab_p) > 0.0001f)
        {
            Maths::Vector3 p_co = plane.normal_ * (-plane.Distance(Maths::Vector3(0.0f)));

            Maths::Vector3 w = start - p_co;
            float fac = -(plane.normal_.DotProduct(w)) / ab_p;
            ab = ab * fac;

            return start + ab;
        }

        return start;
    }

    void CollisionDetection::SutherlandHodgesonClipping(Maths::Vector3* input_polygon, int input_polygon_count, int num_clip_planes, const Maths::Plane* clip_planes, Maths::Vector3* output_polygon, int& output_polygon_count, bool removePoints) const
    {
        LUMOS_PROFILE_FUNCTION();
        if(!output_polygon)
            return;

        Maths::Vector3 ppPolygon1[8], ppPolygon2[8];
        int inputCount = 0, outputCount = 0;

        Maths::Vector3 *input = ppPolygon1, *output = ppPolygon2;
        inputCount = input_polygon_count;
        output = input_polygon;
        outputCount = inputCount;

        for(int iterations = 0; iterations < num_clip_planes; ++iterations)
        {
            if(outputCount == 0)
                break;

            const Maths::Plane& plane = clip_planes[iterations];

            std::swap(input, output);
            inputCount = outputCount;

            outputCount = 0;

            Maths::Vector3 startPoint = input[inputCount - 1];
            for(int i = 0; i < inputCount; i++)
            {
                const auto& endPoint = input[i];
                bool startInPlane = plane.PointInPlane(startPoint);
                bool endInPlane = plane.PointInPlane(endPoint);

                if(removePoints)
                {
                    if(endInPlane)
                        output[outputCount++] = endPoint;
                }
                else
                {
                    //if entire edge is within the clipping plane, keep it as it is
                    if(startInPlane && endInPlane)
                        output[outputCount++] = endPoint;

                    //if edge interesects the clipping plane, cut the edge along clip plane
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

        output_polygon = output;
        output_polygon_count = outputCount;
    }
}
