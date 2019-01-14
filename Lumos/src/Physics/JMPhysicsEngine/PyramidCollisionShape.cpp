#include "JM.h"
#include "PyramidCollisionShape.h"
#include "PhysicsObject3D.h"

namespace jm
{

	Hull* PyramidCollisionShape::m_PyramidHull = new Hull();

	PyramidCollisionShape::PyramidCollisionShape()
	{
		m_PyramidHalfDimensions = maths::Vector3(0.5f, 0.5f, 0.5f);
		m_Type = CollisionShapeType::CollisionPyramid;
		m_LocalTransform = maths::Matrix4::Scale(m_PyramidHalfDimensions);

		if (m_PyramidHull->GetNumVertices() == 0)
		{
			ConstructPyramidHull();
		}
	}

	PyramidCollisionShape::PyramidCollisionShape(const maths::Vector3& halfdims)
	{
		m_PyramidHalfDimensions = halfdims;

		m_LocalTransform = maths::Matrix4::Scale(m_PyramidHalfDimensions);
		m_Type = CollisionShapeType::CollisionPyramid;

		if (!m_PyramidHull)
		{
			m_PyramidHull = new Hull();
		}

		maths::Vector3 m_Points[5] = {
			m_LocalTransform * maths::Vector3(-1.0f, -1.0f, -1.0f),
			m_LocalTransform * maths::Vector3(-1.0f, -1.0f, 1.0f),
			m_LocalTransform * maths::Vector3(1.0f, -1.0f, 1.0f),
			m_LocalTransform * maths::Vector3(1.0f, -1.0f, -1.0f),
			m_LocalTransform * maths::Vector3(0.0f, 1.0f, 0.0f)
		};

		m_Normals[0] = maths::Vector3::Cross(m_Points[0] - m_Points[3], m_Points[4] - m_Points[3]).Normal();
		m_Normals[1] = maths::Vector3::Cross(m_Points[1] - m_Points[0], m_Points[4] - m_Points[0]).Normal();
		m_Normals[2] = maths::Vector3::Cross(m_Points[2] - m_Points[1], m_Points[4] - m_Points[1]).Normal();
		m_Normals[3] = maths::Vector3::Cross(m_Points[3] - m_Points[2], m_Points[4] - m_Points[2]).Normal();
		m_Normals[4] = maths::Vector3(0.0f, -1.0f, 0.0f);

		if (m_PyramidHull->GetNumVertices() == 0)
		{
			ConstructPyramidHull();
		}
	}

	PyramidCollisionShape::~PyramidCollisionShape()
	{
		if (m_PyramidHull != nullptr)
		{
			delete m_PyramidHull;
			m_PyramidHull = nullptr;
		}
	}

	maths::Matrix3 PyramidCollisionShape::BuildInverseInertia(float invMass) const
	{
		maths::Vector3 scaleSq = m_PyramidHalfDimensions * m_PyramidHalfDimensions;

		maths::Matrix3 inertia;

		inertia._11 = invMass / ((0.2f * scaleSq.x) + (0.15f * scaleSq.y));
		inertia._22 = invMass / ((0.2f * scaleSq.z) + (0.15f * scaleSq.y));
		inertia._33 = invMass / ((0.2f * scaleSq.z) + (0.2f * scaleSq.x));

		return inertia;
	}

	void PyramidCollisionShape::GetCollisionAxes(const PhysicsObject3D* currentObject, std::vector<maths::Vector3>* out_axes) const
	{
		if (out_axes)
		{
			const maths::Matrix3 objOrientation = currentObject->GetOrientation().ToMatrix3();
			out_axes->push_back(objOrientation * m_Normals[0]);
			out_axes->push_back(objOrientation * m_Normals[1]);
			out_axes->push_back(objOrientation * m_Normals[2]);
			out_axes->push_back(objOrientation * m_Normals[3]);
			out_axes->push_back(objOrientation * m_Normals[4]);
		}
	}

	void PyramidCollisionShape::GetEdges(const PhysicsObject3D* currentObject, std::vector<CollisionEdge>* out_edges) const
	{
		if (out_edges)
		{
			maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * maths::Matrix4::Scale(maths::Vector3(m_PyramidHalfDimensions));
			for (unsigned int i = 0; i < m_PyramidHull->GetNumEdges(); ++i)
			{
				const HullEdge& edge = m_PyramidHull->GetEdge(i);
				maths::Vector3 A = transform * m_PyramidHull->GetVertex(edge.vStart).pos;
				maths::Vector3 B = transform * m_PyramidHull->GetVertex(edge.vEnd).pos;

				out_edges->emplace_back(A, B);
			}
		}
	}

	void PyramidCollisionShape::GetMinMaxVertexOnAxis(const PhysicsObject3D* currentObject, const maths::Vector3& axis, maths::Vector3* out_min, maths::Vector3* out_max) const
	{
		maths::Matrix4 wsTransform;

		if (currentObject == nullptr)
			wsTransform = m_LocalTransform;
		else
			wsTransform = currentObject->GetWorldSpaceTransform() * maths::Matrix4::Scale(m_PyramidHalfDimensions);

		const maths::Matrix3 invNormalMatrix = maths::Matrix3::Transpose(maths::Matrix3(wsTransform));
		const maths::Vector3 local_axis = invNormalMatrix * axis;

		int vMin, vMax;
		m_PyramidHull->GetMinMaxVerticesInAxis(local_axis, &vMin, &vMax);

		if (out_min) *out_min = wsTransform * m_PyramidHull->GetVertex(vMin).pos;
		if (out_max) *out_max = wsTransform * m_PyramidHull->GetVertex(vMax).pos;
	}

	void PyramidCollisionShape::GetIncidentReferencePolygon(const PhysicsObject3D* currentObject, const maths::Vector3& axis, std::list<maths::Vector3>* out_face, maths::Vector3* out_normal, std::vector<maths::Plane>* out_adjacent_planes) const
	{
		maths::Matrix4 wsTransform;

		if (currentObject == nullptr)
			wsTransform = m_LocalTransform;
		else
			wsTransform = currentObject->GetWorldSpaceTransform() * maths::Matrix4::Scale(m_PyramidHalfDimensions);

		const maths::Matrix3 invNormalMatrix = maths::Matrix3::Inverse(maths::Matrix3(wsTransform));
		const maths::Matrix3 normalMatrix = maths::Matrix3::Transpose(invNormalMatrix);

		const maths::Vector3 local_axis = invNormalMatrix * axis;

		int minVertex, maxVertex;
		m_PyramidHull->GetMinMaxVerticesInAxis(local_axis, &minVertex, &maxVertex);

		const HullVertex& vert = m_PyramidHull->GetVertex(maxVertex);

		const HullFace* best_face = nullptr;
		float best_correlation = -FLT_MAX;
		for (int faceIdx : vert.enclosing_faces)
		{
			const HullFace* face = &m_PyramidHull->GetFace(faceIdx);
			const float temp_correlation = maths::Vector3::Dot(local_axis, face->normal);
			if (temp_correlation > best_correlation)
			{
				best_correlation = temp_correlation;
				best_face = face;
			}
		}

		if (out_normal)
		{
			if(best_face)
				*out_normal = normalMatrix * best_face->normal;
			(*out_normal).Normalise();
		}

		if (out_face && best_face)
		{
			for (int vertIdx : best_face->vert_ids)
			{
				const HullVertex& vertex = m_PyramidHull->GetVertex(vertIdx);
				out_face->push_back(wsTransform * vertex.pos);
			}
		}

		if (out_adjacent_planes && best_face != nullptr)
		{
			//Add the reference face itself to the list of adjacent planes
			maths::Vector3 wsPointOnPlane = wsTransform * m_PyramidHull->GetVertex(m_PyramidHull->GetEdge(best_face->edge_ids[0]).vStart).pos;
			maths::Vector3 planeNrml = -(normalMatrix * best_face->normal);
			planeNrml.Normalise();
			float planeDist = -maths::Vector3::Dot(planeNrml, wsPointOnPlane);

			out_adjacent_planes->emplace_back(planeNrml, planeDist);

			for (int edgeIdx : best_face->edge_ids)
			{
				const HullEdge& edge = m_PyramidHull->GetEdge(edgeIdx);

				wsPointOnPlane = wsTransform * m_PyramidHull->GetVertex(edge.vStart).pos;

				for (int adjFaceIdx : edge.enclosing_faces)
				{
					if (adjFaceIdx != best_face->idx)
					{
						const HullFace& adjFace = m_PyramidHull->GetFace(adjFaceIdx);

						planeNrml = -(normalMatrix * adjFace.normal);
						planeNrml.Normalise();
						planeDist = -maths::Vector3::Dot(planeNrml, wsPointOnPlane);

						out_adjacent_planes->emplace_back(planeNrml, planeDist);
					}
				}
			}
		}
	}

	void PyramidCollisionShape::DebugDraw(const PhysicsObject3D* currentObject) const
	{
		const maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * maths::Matrix4::Scale(m_PyramidHalfDimensions);

		m_PyramidHull->DebugDraw(transform);
	}

	void PyramidCollisionShape::ConstructPyramidHull()
	{
		maths::Vector3 v0 = maths::Vector3(-1.0f, -1.0f, -1.0f);		// 0
		maths::Vector3 v1 = maths::Vector3(-1.0f, -1.0f, 1.0f);		// 1
		maths::Vector3 v2 = maths::Vector3(1.0f, -1.0f, 1.0f);		// 2
		maths::Vector3 v3 = maths::Vector3(1.0f, -1.0f, -1.0f);		// 3
		maths::Vector3 v4 = maths::Vector3(0.0f, 1.0f, 0.0f);		// 4
		//Vertices
		m_PyramidHull->AddVertex(v0);		// 0
		m_PyramidHull->AddVertex(v1);		// 1
		m_PyramidHull->AddVertex(v2);		// 2
		m_PyramidHull->AddVertex(v3);		// 3
		m_PyramidHull->AddVertex(v4);		// 4

		int face1[] = { 0, 4, 3 };
		int face2[] = { 1, 4, 0 };
		int face3[] = { 2, 4, 1 };
		int face4[] = { 3, 4, 2 };
		int face5[] = { 0, 3, 2, 1 };

		m_PyramidHull->AddFace(maths::Vector3::Cross((v0 - v3), v4 - v3).Normal(), 3, face1);
		m_PyramidHull->AddFace(maths::Vector3::Cross(v1 - v0, v4 - v0).Normal(), 3, face2);
		m_PyramidHull->AddFace(maths::Vector3::Cross(v2 - v1, v4 - v1).Normal(), 3, face3);
		m_PyramidHull->AddFace(maths::Vector3::Cross(v3 - v2, v4 - v2).Normal(), 3, face4);
		m_PyramidHull->AddFace(maths::Vector3(0.0f, -1.0f, 0.0f), 4, face5);
	}
}