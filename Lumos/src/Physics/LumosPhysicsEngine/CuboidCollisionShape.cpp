#include "LM.h"
#include "CuboidCollisionShape.h"
#include "PhysicsObject3D.h"
#include "Maths/Matrix3.h"

namespace Lumos
{

	Ref<Hull> CuboidCollisionShape::m_CubeHull = CreateRef<Hull>();

	CuboidCollisionShape::CuboidCollisionShape() 
	{
		m_CuboidHalfDimensions = Maths::Vector3(0.5f, 0.5f, 0.5f);
		m_Type = CollisionShapeType::CollisionCuboid;


		if (m_CubeHull->GetNumVertices() == 0)
		{
			ConstructCubeHull();
		}
	}

	CuboidCollisionShape::CuboidCollisionShape(const Maths::Vector3& halfdims)
	{
		m_CuboidHalfDimensions = halfdims;
		m_LocalTransform = Maths::Matrix4::Scale(halfdims);
		m_Type = CollisionShapeType::CollisionCuboid;

	
		if (m_CubeHull->GetNumVertices() == 0)
		{
			ConstructCubeHull();
		}
	}

	CuboidCollisionShape::~CuboidCollisionShape()
	{
	}

	Maths::Matrix3 CuboidCollisionShape::BuildInverseInertia(float invMass) const
	{
		Maths::Matrix3 inertia;

		Maths::Vector3 dimsSq = (m_CuboidHalfDimensions + m_CuboidHalfDimensions);
		dimsSq = dimsSq * dimsSq;

		inertia._11 = 12.f * invMass * 1.f / (dimsSq.GetY() + dimsSq.GetZ());
		inertia._22 = 12.f * invMass * 1.f / (dimsSq.GetX() + dimsSq.GetZ());
		inertia._33 = 12.f * invMass * 1.f / (dimsSq.GetX() + dimsSq.GetY());

		return inertia;
	}

	void CuboidCollisionShape::GetCollisionAxes(const PhysicsObject3D* currentObject, std::vector<Maths::Vector3>* out_axes) const
	{
		if (out_axes)
		{
			Maths::Matrix3 objOrientation = currentObject->GetOrientation().ToMatrix3();
			out_axes->push_back(objOrientation * Maths::Vector3(1.0f, 0.0f, 0.0f)); //X - Axis
			out_axes->push_back(objOrientation * Maths::Vector3(0.0f, 1.0f, 0.0f)); //Y - Axis
			out_axes->push_back(objOrientation * Maths::Vector3(0.0f, 0.0f, 1.0f)); //Z - Axis
		}
	}

	void CuboidCollisionShape::GetEdges(const PhysicsObject3D* currentObject, std::vector<CollisionEdge>* out_edges) const
	{
		if (out_edges)
		{
			Maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * Maths::Matrix4::Scale(Maths::Vector3(m_CuboidHalfDimensions));
			for (unsigned int i = 0; i < m_CubeHull->GetNumEdges(); ++i)
			{
				const HullEdge& edge = m_CubeHull->GetEdge(i);
				Maths::Vector3 A = transform * m_CubeHull->GetVertex(edge.vStart).pos;
				Maths::Vector3 B = transform * m_CubeHull->GetVertex(edge.vEnd).pos;

				out_edges->emplace_back(A, B);
			}
		}
	}

	void CuboidCollisionShape::GetMinMaxVertexOnAxis(const PhysicsObject3D* currentObject, const Maths::Vector3& axis, Maths::Vector3* out_min, Maths::Vector3* out_max) const
	{
		Maths::Matrix4 wsTransform;

		if (currentObject == nullptr)
			wsTransform = m_LocalTransform;
		else
			wsTransform = currentObject->GetWorldSpaceTransform() * Maths::Matrix4::Scale(m_CuboidHalfDimensions);

		Maths::Matrix3 invNormalMatrix = Maths::Matrix3::Transpose(Maths::Matrix3(wsTransform));
		Maths::Vector3 local_axis = invNormalMatrix * axis;

		int vMin, vMax;

		m_CubeHull->GetMinMaxVerticesInAxis(local_axis, &vMin, &vMax);

		if (out_min) *out_min = wsTransform * m_CubeHull->GetVertex(vMin).pos;
		if (out_max) *out_max = wsTransform * m_CubeHull->GetVertex(vMax).pos;
		
	}

	void CuboidCollisionShape::GetIncidentReferencePolygon(const PhysicsObject3D* currentObject, const Maths::Vector3& axis, std::list<Maths::Vector3>* out_face, Maths::Vector3* out_normal, std::vector<Maths::Plane>* out_adjacent_planes) const
	{
		Maths::Matrix4 wsTransform;

		if (currentObject == nullptr)
			wsTransform = m_LocalTransform;
		else
			wsTransform = currentObject->GetWorldSpaceTransform() * Maths::Matrix4::Scale(m_CuboidHalfDimensions);

		Maths::Matrix3 invNormalMatrix = Maths::Matrix3::Inverse(Maths::Matrix3(wsTransform));
		Maths::Matrix3 normalMatrix = Maths::Matrix3::Transpose(invNormalMatrix);

		Maths::Vector3 local_axis = invNormalMatrix * axis;

		int minVertex, maxVertex;
		m_CubeHull->GetMinMaxVerticesInAxis(local_axis, &minVertex, &maxVertex);

		const HullVertex& vert = m_CubeHull->GetVertex(maxVertex);

		const HullFace* best_face = nullptr;
		float best_correlation = -FLT_MAX;
		for (int faceIdx : vert.enclosing_faces)
		{
			const HullFace* face = &m_CubeHull->GetFace(faceIdx);
			float temp_correlation = Maths::Vector3::Dot(local_axis, face->normal);
			if (temp_correlation > best_correlation)
			{
				best_correlation = temp_correlation;
				best_face = face;
			}
		}

		if (out_normal && best_face)
		{
			*out_normal = normalMatrix * best_face->normal;
			(*out_normal).Normalise();
		}

		if (out_face  && best_face)
		{
			for (int vertIdx : best_face->vert_ids)
			{
				const HullVertex& currentVert = m_CubeHull->GetVertex(vertIdx);
				out_face->push_back(wsTransform * currentVert.pos);
			}
		}

		if (out_adjacent_planes && best_face)
		{
			//Add the reference face itself to the list of adjacent planes
			Maths::Vector3 wsPointOnPlane = wsTransform * m_CubeHull->GetVertex(m_CubeHull->GetEdge(best_face->edge_ids[0]).vStart).pos;
			Maths::Vector3 planeNrml = -(normalMatrix * best_face->normal);
			planeNrml.Normalise();
			float planeDist = -Maths::Vector3::Dot(planeNrml, wsPointOnPlane);

			out_adjacent_planes->emplace_back(planeNrml, planeDist);

			for (int edgeIdx : best_face->edge_ids)
			{
				const HullEdge& edge = m_CubeHull->GetEdge(edgeIdx);

				wsPointOnPlane = wsTransform * m_CubeHull->GetVertex(edge.vStart).pos;

				for (int adjFaceIdx : edge.enclosing_faces)
				{
					if (adjFaceIdx != best_face->idx)
					{
						const HullFace& adjFace = m_CubeHull->GetFace(adjFaceIdx);

						planeNrml = -(normalMatrix * adjFace.normal);
						planeNrml.Normalise();
						planeDist = -Maths::Vector3::Dot(planeNrml, wsPointOnPlane);

						out_adjacent_planes->emplace_back(planeNrml, planeDist);
					}
				}
			}
		}
	}

	void CuboidCollisionShape::DebugDraw(const PhysicsObject3D* currentObject) const
	{
		Maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * Maths::Matrix4::Scale(m_CuboidHalfDimensions);

		if (m_CubeHull->GetNumVertices() == 0)
		{
			ConstructCubeHull();
		}

		m_CubeHull->DebugDraw(transform);
	}

	void CuboidCollisionShape::ConstructCubeHull()
	{
		//Vertices
		m_CubeHull->AddVertex(Maths::Vector3(-1.0f, -1.0f, -1.0f));	// 0
		m_CubeHull->AddVertex(Maths::Vector3(-1.0f, 1.0f, -1.0f));		// 1
		m_CubeHull->AddVertex(Maths::Vector3(1.0f, 1.0f, -1.0f));		// 2
		m_CubeHull->AddVertex(Maths::Vector3(1.0f, -1.0f, -1.0f));		// 3

		m_CubeHull->AddVertex(Maths::Vector3(-1.0f, -1.0f, 1.0f));		// 4
		m_CubeHull->AddVertex(Maths::Vector3(-1.0f, 1.0f, 1.0f));		// 5
		m_CubeHull->AddVertex(Maths::Vector3(1.0f, 1.0f, 1.0f));		// 6
		m_CubeHull->AddVertex(Maths::Vector3(1.0f, -1.0f, 1.0f));		// 7

		int face1[] = { 0, 1, 2, 3 };
		int face2[] = { 7, 6, 5, 4 };
		int face3[] = { 5, 6, 2, 1 };
		int face4[] = { 0, 3, 7, 4 };
		int face5[] = { 6, 7, 3, 2 };
		int face6[] = { 4, 5, 1, 0 };

		//Faces
		m_CubeHull->AddFace(Maths::Vector3(0.0f, 0.0f, -1.0f), 4, face1);
		m_CubeHull->AddFace(Maths::Vector3(0.0f, 0.0f, 1.0f), 4, face2);
		m_CubeHull->AddFace(Maths::Vector3(0.0f, 1.0f, 0.0f), 4, face3);
		m_CubeHull->AddFace(Maths::Vector3(0.0f, -1.0f, 0.0f), 4, face4);
		m_CubeHull->AddFace(Maths::Vector3(1.0f, 0.0f, 0.0f), 4, face5);
		m_CubeHull->AddFace(Maths::Vector3(-1.0f, 0.0f, 0.0f), 4, face6);
	}
}
