#include "Precompiled.h"
#include "CollisionDetection.h"

#include "SphereCollisionShape.h"

namespace Lumos
{
	
	CollisionDetection::CollisionDetection()
	{
		//max actual size
		unsigned int maxSize = CollisionShapeTypeMax | (CollisionShapeTypeMax >> 1);
		m_CollisionCheckFunctions = new CollisionCheckFunc[maxSize];
		std::fill(m_CollisionCheckFunctions, m_CollisionCheckFunctions + maxSize, &CollisionDetection::InvalidCheckCollision);
		
		m_CollisionCheckFunctions[CollisionSphere] = &CollisionDetection::CheckSphereCollision;
		m_CollisionCheckFunctions[CollisionCuboid] = &CollisionDetection::CheckPolyhedronCollision;
		m_CollisionCheckFunctions[CollisionSphere | CollisionCuboid] = &CollisionDetection::CheckPolyhedronSphereCollision;
	}
	
	bool CollisionDetection::InvalidCheckCollision(  RigidBody3D* obj1,   RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
	{
		LUMOS_LOG_CRITICAL("Invalid Collision type specified");
		return false;
	}
	
	bool CollisionDetection::CheckSphereCollision(  RigidBody3D* obj1,   RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
	{
		CollisionData colData;
		Maths::Vector3 axis = obj2->GetPosition() - obj1->GetPosition();
		axis.Normalize();
		if(!CheckCollisionAxis(axis, obj1, obj2, shape1, shape2, &colData))
			return false;
		
		if(out_coldata)
			*out_coldata = colData;
		
		return true;
	}
	
	void AddPossibleCollisionAxis(Maths::Vector3& axis, std::vector<Maths::Vector3>* possible_collision_axes)
	{
		const float epsilon = 0.0001f;
		
		if(axis.LengthSquared() < epsilon)
			return;
		
		axis.Normalize();
		
		for(const Maths::Vector3& p_axis : *possible_collision_axes)
		{
			if(abs(Maths::Vector3::Dot(axis, p_axis)) >= (1.0f - epsilon))
				return;
		}
		
		possible_collision_axes->push_back(axis);
	}
	
	bool CollisionDetection::CheckPolyhedronSphereCollision(RigidBody3D* obj1, RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
	{
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
		
        std::vector<Maths::Vector3>& possibleCollisionAxes = complexShape->GetCollisionAxes(complexObj);
        std::vector<CollisionEdge>& complex_shape_edges = complexShape->GetEdges(complexObj);
		
		Maths::Vector3 p = GetClosestPointOnEdges(sphereObj->GetPosition(), complex_shape_edges);
		Maths::Vector3 p_t = sphereObj->GetPosition() - p;
		p_t.Normalize();
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
	
	bool CollisionDetection::CheckPolyhedronCollision(  RigidBody3D* obj1,   RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
	{
		CollisionData cur_colData;
		CollisionData best_colData;
		best_colData.penetration = -FLT_MAX;
		
        std::vector<Maths::Vector3>& possibleCollisionAxes = shape1->GetCollisionAxes(obj1);
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
				e1.Normalize();
				e2.Normalize();
				
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
	
	bool CollisionDetection::CheckCollisionAxis(const Maths::Vector3& axis,   RigidBody3D* obj1,   RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* out_coldata)
	{
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
	
	bool CollisionDetection::BuildCollisionManifold( RigidBody3D* obj1,   RigidBody3D* obj2, CollisionShape* shape1, CollisionShape* shape2, CollisionData& coldata, Manifold* manifold)
	{
        LUMOS_PROFILE_FUNCTION();
		if(!manifold)
			return false;
		
		std::list<Maths::Vector3> polygon1, polygon2;
		Maths::Vector3 normal1, normal2;
		std::vector<Maths::Plane> adjPlanes1, adjPlanes2;
		adjPlanes1.reserve(20);
		adjPlanes2.reserve(20);
		shape1->GetIncidentReferencePolygon(obj1, coldata.normal, &polygon1, &normal1, &adjPlanes1);
		shape2->GetIncidentReferencePolygon(obj2, -coldata.normal, &polygon2, &normal2, &adjPlanes2);
		
		if(polygon1.empty() || polygon2.empty())
			return false;
		else if(polygon1.size() == 1)
			manifold->AddContact(polygon1.front(), polygon1.front() - coldata.normal * coldata.penetration, coldata.normal, coldata.penetration);
		else if(polygon2.size() == 1)
			manifold->AddContact(polygon2.front() + coldata.normal * coldata.penetration, polygon2.front(), coldata.normal, coldata.penetration);
		else
		{
			bool flipped;
			std::list<Maths::Vector3>* incPolygon;
			std::vector<Maths::Plane>* refAdjPlanes;
			Maths::Plane refPlane;
			
			if(fabs(coldata.normal.DotProduct(normal1)) > fabs(coldata.normal.DotProduct(normal2)))
			{
				float planeDist = -(polygon1.front().DotProduct(-normal1));
				refPlane = Maths::Plane(-normal1, planeDist);
				refAdjPlanes = &adjPlanes1;
				
				incPolygon = &polygon2;
				
				flipped = false;
			}
			else
			{
				float planeDist = -(polygon2.front().DotProduct(-normal2));
				refPlane = Maths::Plane(-normal2, planeDist);
				refAdjPlanes = &adjPlanes2;
				
				incPolygon = &polygon1;
				
				flipped = true;
			}
			
			SutherlandHodgesonClipping(*incPolygon, static_cast<int>(refAdjPlanes->size()), &(*refAdjPlanes)[0], incPolygon, false);
			
			SutherlandHodgesonClipping(*incPolygon, 1, &refPlane, incPolygon, true);
			
			for(const Maths::Vector3& endPoint : *incPolygon)
			{
				float contact_penetration;
				Maths::Vector3 globalOnA, globalOnB;
				
				if(flipped)
				{
					contact_penetration =
						-(endPoint.DotProduct(coldata.normal)
						  - (coldata.normal.DotProduct(polygon2.front())));
					
					globalOnA = endPoint + coldata.normal * contact_penetration;
					globalOnB = endPoint;
				}
				else
				{
					contact_penetration = endPoint.DotProduct(coldata.normal) - coldata.normal.DotProduct(polygon1.front());
					
					globalOnA = endPoint;
					globalOnB = endPoint - coldata.normal * contact_penetration;
				}
				
				manifold->AddContact(globalOnA, globalOnB, coldata.normal, contact_penetration);
			}
		}
		return true;
	}
	
	Maths::Vector3 CollisionDetection::GetClosestPointOnEdges(const Maths::Vector3& target, const std::vector<CollisionEdge>& edges)
	{
		Maths::Vector3 closest_point, temp_closest_point;
		float closest_distsq = FLT_MAX;
		
		for(const CollisionEdge& edge : edges)
		{
			Maths::Vector3 a_t = target - edge.posA;
			Maths::Vector3 a_b = edge.posB - edge.posA;
			
			float magnitudeAB = a_b.DotProduct(a_b); //Magnitude of AB vector (it's length squared)
			float ABAPproduct = a_t.DotProduct(a_b); //The DOT product of a_to_t and a_to_b
			float distance = ABAPproduct / magnitudeAB; //The normalized "distance" from a to your closest point
			
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
	
	void CollisionDetection::SutherlandHodgesonClipping(const std::list<Maths::Vector3>& input_polygon, int num_clip_planes, const Maths::Plane* clip_planes, std::list<Maths::Vector3>* out_polygon, bool removePoints) const
	{
        LUMOS_PROFILE_FUNCTION();
		if(!out_polygon)
			return;
		
		std::list<Maths::Vector3> ppPolygon1, ppPolygon2;
		std::list<Maths::Vector3>*input = &ppPolygon1, *output = &ppPolygon2;
		
		*output = input_polygon;
		for(int iterations = 0; iterations < num_clip_planes; ++iterations)
		{
			if(output->empty())
				break;
			
			const Maths::Plane& plane = clip_planes[iterations];
			
			std::swap(input, output);
			output->clear();
			
			Maths::Vector3 startPoint = input->back();
			for(const Maths::Vector3& endPoint : *input)
			{
				bool startInPlane = plane.PointInPlane(startPoint);
				bool endInPlane = plane.PointInPlane(endPoint);
				
				if(removePoints)
				{
					if(endInPlane)
						output->push_back(endPoint);
				}
				else
				{
					//if entire edge is within the clipping plane, keep it as it is
					if(startInPlane && endInPlane)
						output->push_back(endPoint);
					
					//if edge interesects the clipping plane, cut the edge along clip plane
					else if(startInPlane && !endInPlane)
						output->push_back(PlaneEdgeIntersection(plane, startPoint, endPoint));
					else if(!startInPlane && endInPlane)
					{
						output->push_back(PlaneEdgeIntersection(plane, endPoint, startPoint));
						output->push_back(endPoint);
					}
				}
				
				startPoint = endPoint;
			}
		}
		
		*out_polygon = *output;
	}
}
