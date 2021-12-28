#include "Precompiled.h"
#include "CollisionDetection.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/SphereCollisionShape.h"

namespace Lumos
{
    CollisionDetection::CollisionDetection()
    {
        m_MaxSize = CollisionShapeTypeMax | (CollisionShapeTypeMax >> 1);
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

        CollisionData colData;
        glm::vec3 axis = obj2->GetPosition() - obj1->GetPosition();
        axis = glm::normalize(axis);
        if(!CheckCollisionAxis(axis, obj1, obj2, shape1, shape2, &colData))
            return false;

        if(out_coldata)
            *out_coldata = colData;

        return true;
    }

    void AddPossibleCollisionAxis(glm::vec3& axis, glm::vec3* possibleCollisionAxes, uint32_t& possibleCollisionAxesCount)
    {
        LUMOS_PROFILE_FUNCTION();
        const float epsilon = 0.0001f;

        if(glm::length2(axis) < epsilon)
            return;

        axis = glm::normalize(axis);

        float value = (1.0f - epsilon);

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

        std::vector<glm::vec3>& shapeCollisionAxes = complexShape->GetCollisionAxes(complexObj);
        std::vector<CollisionEdge>& complex_shape_edges = complexShape->GetEdges(complexObj);

        glm::vec3 p = GetClosestPointOnEdges(sphereObj->GetPosition(), complex_shape_edges);
        glm::vec3 p_t = sphereObj->GetPosition() - p;
        p_t = glm::normalize(p_t);

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

        std::vector<glm::vec3>& shape1CollisionAxes = shape1->GetCollisionAxes(obj1);
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
                e1 = glm::normalize(e1);
                e2 = glm::normalize(e2);

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
            glm::vec3* incPolygon;
            int incPolygonCount;
            Plane* refAdjPlanes;
            int refAdjPlanesCount;
            Plane refPlane;

            if(glm::abs(glm::dot(coldata.normal, poly1.Normal)) > glm::abs(glm::dot(coldata.normal, poly2.Normal)))
            {
                float planeDist = -(glm::dot(poly1.Faces[0], -poly1.Normal));
                refPlane = Plane(-poly1.Normal, planeDist);

                refAdjPlanes = poly1.AdjacentPlanes;
                refAdjPlanesCount = poly1.PlaneCount;
                incPolygon = poly2.Faces;
                incPolygonCount = poly2.FaceCount;

                flipped = false;
            }
            else
            {
                float planeDist = -(glm::dot(poly2.Faces[0], -poly2.Normal));
                refPlane = Plane(-poly2.Normal, planeDist);

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

                //Thread safe here
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

            float magnitudeAB = glm::dot(a_b, a_b); //Magnitude of AB vector (it's length squared)
            float ABAPproduct = glm::dot(a_t, a_b); //The DOT product of a_to_t and a_to_b
            float distance = ABAPproduct / magnitudeAB; //The Normalised "distance" from a to your closest point

            if(distance < 0.0f) //Clamp returned point to be on the line, e.g if the closest point is beyond the AB return either A or B as closest points
                temp_closest_point = edge.posA;

            else if(distance > 1)
                temp_closest_point = edge.posB;
            else
                temp_closest_point = edge.posA + a_b * distance;

            glm::vec3 c_t = target - temp_closest_point;
            float temp_distsq = glm::dot(c_t, c_t);

            if(temp_distsq < closest_distsq)
            {
                closest_distsq = temp_distsq;
                closest_point = temp_closest_point;
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
            float fac = -(glm::dot(plane.Normal(), w)) / ab_p;
            ab = ab * fac;

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
        inputCount = input_polygon_count;
        output = input_polygon;
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
                bool startInPlane = plane.IsPointOnPlane(startPoint);
                bool endInPlane = plane.IsPointOnPlane(endPoint);

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
