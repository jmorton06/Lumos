#include "lmpch.h"
#include "PyramidCollisionShape.h"
#include "PhysicsObject3D.h"

namespace Lumos
{

	Scope<Hull> PyramidCollisionShape::m_PyramidHull  = CreateScope<Hull>();

	PyramidCollisionShape::PyramidCollisionShape()
	{
		m_PyramidHalfDimensions = Maths::Vector3(0.5f, 0.5f, 0.5f);
		m_Type = CollisionShapeType::CollisionPyramid;
		m_LocalTransform = Maths::Matrix4::Scale(m_PyramidHalfDimensions);

		if (m_PyramidHull->GetNumVertices() == 0)
		{
			ConstructPyramidHull();
		}
	}

	PyramidCollisionShape::PyramidCollisionShape(const Maths::Vector3& halfdims)
	{
		m_PyramidHalfDimensions = halfdims;

		m_LocalTransform = Maths::Matrix4::Scale(m_PyramidHalfDimensions);
		m_Type = CollisionShapeType::CollisionPyramid;

		Maths::Vector3 m_Points[5] = {
			m_LocalTransform * Maths::Vector3(-1.0f, -1.0f, -1.0f),
			m_LocalTransform * Maths::Vector3(-1.0f, -1.0f, 1.0f),
			m_LocalTransform * Maths::Vector3(1.0f, -1.0f, 1.0f),
			m_LocalTransform * Maths::Vector3(1.0f, -1.0f, -1.0f),
			m_LocalTransform * Maths::Vector3(0.0f, 1.0f, 0.0f)
		};

		m_Normals[0] = Maths::Vector3::Cross(m_Points[0] - m_Points[3], m_Points[4] - m_Points[3]).Normalized();
		m_Normals[1] = Maths::Vector3::Cross(m_Points[1] - m_Points[0], m_Points[4] - m_Points[0]).Normalized();
		m_Normals[2] = Maths::Vector3::Cross(m_Points[2] - m_Points[1], m_Points[4] - m_Points[1]).Normalized();
		m_Normals[3] = Maths::Vector3::Cross(m_Points[3] - m_Points[2], m_Points[4] - m_Points[2]).Normalized();
		m_Normals[4] = Maths::Vector3(0.0f, -1.0f, 0.0f);

		if (m_PyramidHull->GetNumVertices() == 0)
		{
			ConstructPyramidHull();
		}
	}

	PyramidCollisionShape::~PyramidCollisionShape()
	{
	}

	Maths::Matrix3 PyramidCollisionShape::BuildInverseInertia(float invMass) const
	{
		Maths::Vector3 scaleSq = m_PyramidHalfDimensions * m_PyramidHalfDimensions;

		Maths::Matrix3 inertia;

		inertia.m00_ = invMass / ((0.2f * scaleSq.x) + (0.15f * scaleSq.y));
		inertia.m11_ = invMass / ((0.2f * scaleSq.z) + (0.15f * scaleSq.y));
		inertia.m22_ = invMass / ((0.2f * scaleSq.z) + (0.2f * scaleSq.x));

		return inertia;
	}

	void PyramidCollisionShape::ColumnlisionAxes(const PhysicsObject3D* currentObject, std::vector<Maths::Vector3>* out_axes) const
	{
		if (out_axes)
		{
			const Maths::Matrix3 objOrientation = currentObject->GetOrientation().RotationMatrix();
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
			Maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * Maths::Matrix4::Scale(Maths::Vector3(m_PyramidHalfDimensions));
			for (unsigned int i = 0; i < m_PyramidHull->GetNumEdges(); ++i)
			{
				const HullEdge& edge = m_PyramidHull->GetEdge(i);
				Maths::Vector3 A = transform * m_PyramidHull->GetVertex(edge.vStart).pos;
				Maths::Vector3 B = transform * m_PyramidHull->GetVertex(edge.vEnd).pos;

				out_edges->emplace_back(A, B);
			}
		}
	}

	void PyramidCollisionShape::GetMinMaxVertexOnAxis(const PhysicsObject3D* currentObject, const Maths::Vector3& axis, Maths::Vector3* out_min, Maths::Vector3* out_max) const
	{
		Maths::Matrix4 wsTransform;

		if (currentObject == nullptr)
			wsTransform = m_LocalTransform;
		else
			wsTransform = currentObject->GetWorldSpaceTransform() * Maths::Matrix4::Scale(m_PyramidHalfDimensions);

		const Maths::Matrix3 invNormalMatrix = wsTransform.ToMatrix3().Transpose();
		const Maths::Vector3 local_axis = invNormalMatrix * axis;

		int vMin, vMax;
		m_PyramidHull->GetMinMaxVerticesInAxis(local_axis, &vMin, &vMax);

		if (out_min) *out_min = wsTransform * m_PyramidHull->GetVertex(vMin).pos;
		if (out_max) *out_max = wsTransform * m_PyramidHull->GetVertex(vMax).pos;
	}

	void PyramidCollisionShape::GetIncidentReferencePolygon(const PhysicsObject3D* currentObject, const Maths::Vector3& axis, std::list<Maths::Vector3>* out_face, Maths::Vector3* out_normal, std::vector<Maths::Plane>* out_adjacent_planes) const
	{
		Maths::Matrix4 wsTransform;

		if (currentObject == nullptr)
			wsTransform = m_LocalTransform;
		else
			wsTransform = currentObject->GetWorldSpaceTransform() * Maths::Matrix4::Scale(m_PyramidHalfDimensions);

		const Maths::Matrix3 invNormalMatrix = wsTransform.ToMatrix3().Inverse();
		const Maths::Matrix3 normalMatrix = invNormalMatrix.Transpose();

		const Maths::Vector3 local_axis = invNormalMatrix * axis;

		int minVertex, maxVertex;
		m_PyramidHull->GetMinMaxVerticesInAxis(local_axis, &minVertex, &maxVertex);

		const HullVertex& vert = m_PyramidHull->GetVertex(maxVertex);

		const HullFace* best_face = nullptr;
		float best_correlation = -FLT_MAX;
		for (int faceIdx : vert.enclosing_faces)
		{
			const HullFace* face = &m_PyramidHull->GetFace(faceIdx);
			const float temp_correlation = Maths::Vector3::Dot(local_axis, face->normal);
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
			(*out_normal).Normalize();
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
			Maths::Vector3 wsPointOnPlane = wsTransform * m_PyramidHull->GetVertex(m_PyramidHull->GetEdge(best_face->edge_ids[0]).vStart).pos;
			Maths::Vector3 planeNrml = -(normalMatrix * best_face->normal);
			planeNrml.Normalize();
			float planeDist = -Maths::Vector3::Dot(planeNrml, wsPointOnPlane);

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
						planeNrml.Normalize();
						planeDist = -Maths::Vector3::Dot(planeNrml, wsPointOnPlane);

						out_adjacent_planes->emplace_back(planeNrml, planeDist);
					}
				}
			}
		}
	}

	void PyramidCollisionShape::DebugDraw(const PhysicsObject3D* currentObject) const
	{
		const Maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * Maths::Matrix4::Scale(m_PyramidHalfDimensions);

		m_PyramidHull->DebugDraw(transform);
	}

	void PyramidCollisionShape::ConstructPyramidHull()
	{
		Maths::Vector3 v0 = Maths::Vector3(-1.0f, -1.0f, -1.0f);		// 0
		Maths::Vector3 v1 = Maths::Vector3(-1.0f, -1.0f, 1.0f);		// 1
		Maths::Vector3 v2 = Maths::Vector3(1.0f, -1.0f, 1.0f);		// 2
		Maths::Vector3 v3 = Maths::Vector3(1.0f, -1.0f, -1.0f);		// 3
		Maths::Vector3 v4 = Maths::Vector3(0.0f, 1.0f, 0.0f);		// 4
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

		m_PyramidHull->AddFace(Maths::Vector3::Cross((v0 - v3), v4 - v3).Normalized(), 3, face1);
		m_PyramidHull->AddFace(Maths::Vector3::Cross(v1 - v0, v4 - v0).Normalized(), 3, face2);
		m_PyramidHull->AddFace(Maths::Vector3::Cross(v2 - v1, v4 - v1).Normalized(), 3, face3);
		m_PyramidHull->AddFace(Maths::Vector3::Cross(v3 - v2, v4 - v2).Normalized(), 3, face4);
		m_PyramidHull->AddFace(Maths::Vector3(0.0f, -1.0f, 0.0f), 4, face5);
	}
}
