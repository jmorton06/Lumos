#include "JM.h"
#include "CollisionDetection.h"

#include "SphereCollisionShape.h"

namespace jm
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


	bool CollisionDetection::InvalidCheckCollision(const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, CollisionData* out_coldata) const
	{
		JM_ERROR("Invalid Collision type specified");
		return false;
	}

	bool CollisionDetection::CheckSphereCollision(const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, CollisionData* out_coldata) const
	{
		//const SphereCollisionShape* sphere1 = reinterpret_cast<const SphereCollisionShape*>(shape1);
		//const SphereCollisionShape* sphere2 = reinterpret_cast<const SphereCollisionShape*>(shape2);

		CollisionData colData;
		maths::Vector3 axis = obj2->GetPosition() - obj1->GetPosition();
		axis.Normalise();
		if (!CheckCollisionAxis(axis, obj1, obj2, shape1, shape2, &colData))
			return false;

		if (out_coldata)
			*out_coldata = colData;

		return true;
	}

	void AddPossibleCollisionAxis(maths::Vector3& axis, std::vector<maths::Vector3>* possible_collision_axes)
	{
		const float epsilon = 0.0001f;

		if (axis.LengthSquared() < epsilon)
			return;

		axis.Normalise();

		for (const maths::Vector3& p_axis : *possible_collision_axes)
		{
			if (abs(maths::Vector3::Dot(axis,p_axis)) >= (1.0f - epsilon))
				return;
		}

		possible_collision_axes->push_back(axis);
	}


	bool CollisionDetection::CheckPolyhedronSphereCollision(const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, CollisionData* out_coldata) const 
	{
		const SphereCollisionShape* sphereShape;
		const CollisionShape* complexShape;
		const PhysicsObject3D* complexObj;
		const PhysicsObject3D* sphereObj;

		if (obj1->GetCollisionShape()->GetType() == CollisionShapeType::CollisionSphere) 
		{
			sphereShape = reinterpret_cast<const SphereCollisionShape*>(shape1);
			sphereObj = obj1;
			complexShape = shape2;
			complexObj = obj2;
		}
		else 
		{
			sphereShape = reinterpret_cast<const SphereCollisionShape*>(shape2);
			sphereObj = obj2;
			complexShape = shape1;
			complexObj = obj1;
		}

		CollisionData cur_colData;
		CollisionData best_colData;
		best_colData.penetration = -FLT_MAX;

		std::vector<maths::Vector3> possibleCollisionAxes;
		complexShape->GetCollisionAxes(complexObj, &possibleCollisionAxes);

		std::vector<CollisionEdge> complex_shape_edges;

		complexShape->GetEdges(complexObj, &complex_shape_edges);

		maths::Vector3 p = GetClosestPointOnEdges(sphereObj->GetPosition(), complex_shape_edges);
		//NCLDebug::DrawPoint(p, 0.1f);
		maths::Vector3 p_t = sphereObj->GetPosition() - p;
		p_t.Normalise();
		AddPossibleCollisionAxis(p_t, &possibleCollisionAxes);

		for (const maths::Vector3& axis : possibleCollisionAxes)
		{
			if (!CheckCollisionAxis(axis, obj1, obj2, shape1, shape2, &cur_colData))
				return false;

			if (cur_colData.penetration > best_colData.penetration)
				best_colData = cur_colData;
		}

		if (out_coldata)
			*out_coldata = best_colData;

		return true;
	}

	bool CollisionDetection::CheckPolyhedronCollision(const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, CollisionData* out_coldata) const
	{
		CollisionData cur_colData;
		CollisionData best_colData;
		best_colData.penetration = -FLT_MAX;

		std::vector<maths::Vector3> possibleCollisionAxes;
		std::vector<maths::Vector3> tempPossibleCollisionAxes;
		shape1->GetCollisionAxes(obj1, &possibleCollisionAxes);
		shape2->GetCollisionAxes(obj2, &tempPossibleCollisionAxes);
		for (maths::Vector3& temp : tempPossibleCollisionAxes)
			AddPossibleCollisionAxis(temp, &possibleCollisionAxes);

		std::vector<CollisionEdge> shape1_edges;
		std::vector<CollisionEdge> shape2_edges;

		shape1->GetEdges(obj1, &shape1_edges);
		shape2->GetEdges(obj2, &shape2_edges);

		for (const CollisionEdge& edge1 : shape1_edges) 
		{
			for (const CollisionEdge& edge2 : shape2_edges) 
			{
				maths::Vector3 e1 = edge1.posB - edge1.posA;
				maths::Vector3 e2 = edge2.posB - edge2.posA;
				e1.Normalise();
				e2.Normalise();

				maths::Vector3 temp = e1.Cross(e2);
				AddPossibleCollisionAxis(temp, &possibleCollisionAxes);
			}
		}

		for (const maths::Vector3& axis : possibleCollisionAxes)
		{
			if (!CheckCollisionAxis(axis, obj1, obj2, shape1, shape2, &cur_colData))
				return false;

			if (cur_colData.penetration >= best_colData.penetration)
				best_colData = cur_colData;
		}

		if (out_coldata)
			*out_coldata = best_colData;

		return true;
	}

	bool CollisionDetection::CheckCollisionAxis(const maths::Vector3& axis, const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, CollisionData* out_coldata)
	{
		maths::Vector3 min1, min2, max1, max2;

		shape1->GetMinMaxVertexOnAxis(obj1, axis, &min1, &max1);
		shape2->GetMinMaxVertexOnAxis(obj2, axis, &min2, &max2);

		float minCorrelation1 = axis.Dot(min1);
		float maxCorrelation1 = axis.Dot(max1);
		float minCorrelation2 = axis.Dot(min2);
		float maxCorrelation2 = axis.Dot(max2);

		if (minCorrelation1 <= minCorrelation2 && maxCorrelation1 >= minCorrelation2)
		{
			if (out_coldata != nullptr) 
			{
				out_coldata->normal = axis;
				out_coldata->penetration = minCorrelation2 - maxCorrelation1;
				out_coldata->pointOnPlane = max1 + out_coldata->normal * out_coldata->penetration;
			}
			return true;
		}

		if (minCorrelation2 <= minCorrelation1 && maxCorrelation2 > minCorrelation1) 
		{
			if (out_coldata != nullptr) 
			{
				out_coldata->normal = -axis;
				out_coldata->penetration = minCorrelation1 - maxCorrelation2;
				out_coldata->pointOnPlane = min1 + out_coldata->normal * out_coldata->penetration;
			}

			return true;
		}

		return false;
	}

	bool CollisionDetection::BuildCollisionManifold(const PhysicsObject3D* obj1, const PhysicsObject3D* obj2, const CollisionShape* shape1, const CollisionShape* shape2, const CollisionData& coldata, Manifold* manifold) const
	{
		if (!manifold)
			return false;

		std::list<maths::Vector3> polygon1, polygon2;
		maths::Vector3 normal1, normal2;
		std::vector<maths::Plane> adjPlanes1, adjPlanes2;

		shape1->GetIncidentReferencePolygon(obj1, coldata.normal, &polygon1, &normal1, &adjPlanes1);
		shape2->GetIncidentReferencePolygon(obj2, -coldata.normal, &polygon2, &normal2, &adjPlanes2);

		if (polygon1.size() == 0 || polygon2.size() == 0)
			return false;
		else if (polygon1.size() == 1)
			manifold->AddContact(polygon1.front(), polygon1.front() - coldata.normal * coldata.penetration, coldata.normal, coldata.penetration);
		else if (polygon2.size() == 1)
			manifold->AddContact(polygon2.front() + coldata.normal * coldata.penetration, polygon2.front(), coldata.normal, coldata.penetration);
		else 
		{
			bool flipped;
			std::list<maths::Vector3>* incPolygon;
			maths::Vector3* incNormal;
			std::vector<maths::Plane>* refAdjPlanes;
			maths::Plane refPlane;

			if (fabs(coldata.normal.Dot(normal1)) > fabs(coldata.normal.Dot(normal2))) 
			{
				float planeDist = -(polygon1.front().Dot(-normal1));
				refPlane = maths::Plane(-normal1, planeDist);
				refAdjPlanes = &adjPlanes1;

				incPolygon = &polygon2;
				incNormal = &normal2;

				flipped = false;
			}
			else 
			{
				float planeDist = -(polygon2.front().Dot(-normal2));
				refPlane = maths::Plane(-normal2, planeDist);
				refAdjPlanes = &adjPlanes2;

				incPolygon = &polygon1;
				incNormal = &normal1;

				flipped = true;
			}

			SutherlandHodgesonClipping(*incPolygon, static_cast<int>(refAdjPlanes->size()), &(*refAdjPlanes)[0], incPolygon, false);

			SutherlandHodgesonClipping(*incPolygon, 1, &refPlane, incPolygon, true);

			for (const maths::Vector3& endPoint : *incPolygon)
			{
				float contact_penetration;
				maths::Vector3 globalOnA, globalOnB;

				if (flipped) 
				{
					contact_penetration =
						-(endPoint.Dot(coldata.normal)
						- (coldata.normal.Dot(polygon2.front())));

					globalOnA = endPoint + coldata.normal * contact_penetration;
					globalOnB = endPoint;
				}
				else
				{
					contact_penetration = endPoint.Dot(coldata.normal) - coldata.normal.Dot(polygon1.front());

					globalOnA = endPoint;
					globalOnB = endPoint - coldata.normal * contact_penetration;
				}

				manifold->AddContact(globalOnA, globalOnB, coldata.normal, contact_penetration);
			}
		}
		return true;
	}

	maths::Vector3 CollisionDetection::GetClosestPointOnEdges(const maths::Vector3& target, const std::vector<CollisionEdge>& edges)
	{
		maths::Vector3 closest_point, temp_closest_point;
		float closest_distsq = FLT_MAX;

		for (const CollisionEdge& edge : edges) 
		{
			maths::Vector3 a_t = target - edge.posA;
			maths::Vector3 a_b = edge.posB - edge.posA;

			float magnitudeAB = a_b.Dot(a_b);   //Magnitude of AB vector (it's length squared)
			float ABAPproduct = a_t.Dot(a_b);   //The DOT product of a_to_t and a_to_b
			float distance = ABAPproduct / magnitudeAB; //The normalized "distance" from a to your closest point

			if (distance < 0.0f)     //Clamp returned point to be on the line, e.g if the closest point is beyond the AB return either A or B as closest points
				temp_closest_point = edge.posA;

			else if (distance > 1)
				temp_closest_point = edge.posB;
			else
				temp_closest_point = edge.posA + a_b * distance;

			maths::Vector3 c_t = target - temp_closest_point;
			float temp_distsq = c_t.Dot(c_t);

			if (temp_distsq < closest_distsq)
			{
				closest_distsq = temp_distsq;
				closest_point = temp_closest_point;
			}
		}

		return closest_point;
	}

	maths::Vector3 CollisionDetection::PlaneEdgeIntersection(const maths::Plane& plane, const maths::Vector3& start, const maths::Vector3& end) const
	{
		//float start_dist = start.Dot(plane.GetNormal()) + plane.GetDistance();
		//float end_dist = end.Dot(plane.GetNormal()) + plane.GetDistance();

		maths::Vector3 ab = end - start;

		float ab_p = plane.GetNormal().Dot(ab);

		if (fabs(ab_p) > 0.0001f) {
			maths::Vector3 p_co = plane.GetNormal() * (-plane.GetDistance());

			maths::Vector3 w = start - p_co;
			float fac = -(plane.GetNormal().Dot(w)) / ab_p;
			ab = ab * fac;

			return start + ab;
		}

		return start;
	}

	void CollisionDetection::SutherlandHodgesonClipping(const std::list<maths::Vector3>& input_polygon, int num_clip_planes, const maths::Plane* clip_planes, std::list<maths::Vector3>* out_polygon, bool removePoints) const
	{
		if (!out_polygon)
			return;

		std::list<maths::Vector3> ppPolygon1, ppPolygon2;
		std::list<maths::Vector3>* input = &ppPolygon1, *output = &ppPolygon2;

		*output = input_polygon;
		for (int iterations = 0; iterations < num_clip_planes; ++iterations)
		{
			if (output->empty())
				break;

			const maths::Plane& plane = clip_planes[iterations];

			std::swap(input, output);
			output->clear();

			maths::Vector3 startPoint = input->back();
			for (const maths::Vector3& endPoint : *input)
			{
				bool startInPlane = plane.PointInPlane(startPoint);
				bool endInPlane = plane.PointInPlane(endPoint);

				if (removePoints)
				{
					if (endInPlane)
						output->push_back(endPoint);
				}
				else
				{
					//if entire edge is within the clipping plane, keep it as it is
					if (startInPlane && endInPlane)
						output->push_back(endPoint);

					//if edge interesects the clipping plane, cut the edge along clip plane
					else if (startInPlane && !endInPlane)
						output->push_back(PlaneEdgeIntersection(plane, startPoint, endPoint));
					else if (!startInPlane && endInPlane) 
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
