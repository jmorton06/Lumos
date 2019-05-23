#include "LM.h"
#include "CuboidCollisionShape.h"
#include "PhysicsObject3D.h"
#include "Maths/Matrix3.h"

namespace lumos
{

	std::shared_ptr<Hull> CuboidCollisionShape::m_CubeHull = std::make_shared<Hull>();

	CuboidCollisionShape::CuboidCollisionShape() 
	{
		m_CuboidHalfDimensions = maths::Vector3(0.5f, 0.5f, 0.5f);
		m_Type = CollisionShapeType::CollisionCuboid;


		if (m_CubeHull->GetNumVertices() == 0)
		{
			ConstructCubeHull();
		}
	}

	CuboidCollisionShape::CuboidCollisionShape(const maths::Vector3& halfdims)
	{
		m_CuboidHalfDimensions = halfdims;
		m_LocalTransform = maths::Matrix4::Scale(halfdims);
		m_Type = CollisionShapeType::CollisionCuboid;

	
		if (m_CubeHull->GetNumVertices() == 0)
		{
			ConstructCubeHull();
		}
	}

	CuboidCollisionShape::~CuboidCollisionShape()
	{
	}

	maths::Matrix3 CuboidCollisionShape::BuildInverseInertia(float invMass) const
	{
		maths::Matrix3 inertia;

		maths::Vector3 dimsSq = (m_CuboidHalfDimensions + m_CuboidHalfDimensions);
		dimsSq = dimsSq * dimsSq;

		inertia._11 = 12.f * invMass * 1.f / (dimsSq.GetY() + dimsSq.GetZ());
		inertia._22 = 12.f * invMass * 1.f / (dimsSq.GetX() + dimsSq.GetZ());
		inertia._33 = 12.f * invMass * 1.f / (dimsSq.GetX() + dimsSq.GetY());

		return inertia;
	}

	void CuboidCollisionShape::GetCollisionAxes(const PhysicsObject3D* currentObject, std::vector<maths::Vector3>* out_axes) const
	{
		if (out_axes)
		{
			maths::Matrix3 objOrientation = currentObject->GetOrientation().ToMatrix3();
			out_axes->push_back(objOrientation * maths::Vector3(1.0f, 0.0f, 0.0f)); //X - Axis
			out_axes->push_back(objOrientation * maths::Vector3(0.0f, 1.0f, 0.0f)); //Y - Axis
			out_axes->push_back(objOrientation * maths::Vector3(0.0f, 0.0f, 1.0f)); //Z - Axis
		}
	}

	void CuboidCollisionShape::GetEdges(const PhysicsObject3D* currentObject, std::vector<CollisionEdge>* out_edges) const
	{
		if (out_edges)
		{
			maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * maths::Matrix4::Scale(maths::Vector3(m_CuboidHalfDimensions));
			for (unsigned int i = 0; i < m_CubeHull->GetNumEdges(); ++i)
			{
				const HullEdge& edge = m_CubeHull->GetEdge(i);
				maths::Vector3 A = transform * m_CubeHull->GetVertex(edge.vStart).pos;
				maths::Vector3 B = transform * m_CubeHull->GetVertex(edge.vEnd).pos;

				out_edges->emplace_back(A, B);
			}
		}
	}

	void CuboidCollisionShape::GetMinMaxVertexOnAxis(const PhysicsObject3D* currentObject, const maths::Vector3& axis, maths::Vector3* out_min, maths::Vector3* out_max) const
	{
		maths::Matrix4 wsTransform;

		if (currentObject == nullptr)
			wsTransform = m_LocalTransform;
		else
			wsTransform = currentObject->GetWorldSpaceTransform() * maths::Matrix4::Scale(m_CuboidHalfDimensions);

		maths::Matrix3 invNormalMatrix = maths::Matrix3::Transpose(maths::Matrix3(wsTransform));
		maths::Vector3 local_axis = invNormalMatrix * axis;

		int vMin, vMax;

		m_CubeHull->GetMinMaxVerticesInAxis(local_axis, &vMin, &vMax);

		if (out_min) *out_min = wsTransform * m_CubeHull->GetVertex(vMin).pos;
		if (out_max) *out_max = wsTransform * m_CubeHull->GetVertex(vMax).pos;
		
	}

	void CuboidCollisionShape::GetIncidentReferencePolygon(const PhysicsObject3D* currentObject, const maths::Vector3& axis, std::list<maths::Vector3>* out_face, maths::Vector3* out_normal, std::vector<maths::Plane>* out_adjacent_planes) const
	{
		maths::Matrix4 wsTransform;

		if (currentObject == nullptr)
			wsTransform = m_LocalTransform;
		else
			wsTransform = currentObject->GetWorldSpaceTransform() * maths::Matrix4::Scale(m_CuboidHalfDimensions);

		maths::Matrix3 invNormalMatrix = maths::Matrix3::Inverse(maths::Matrix3(wsTransform));
		maths::Matrix3 normalMatrix = maths::Matrix3::Transpose(invNormalMatrix);

		maths::Vector3 local_axis = invNormalMatrix * axis;

		int minVertex, maxVertex;
		m_CubeHull->GetMinMaxVerticesInAxis(local_axis, &minVertex, &maxVertex);

		const HullVertex& vert = m_CubeHull->GetVertex(maxVertex);

		const HullFace* best_face = nullptr;
		float best_correlation = -FLT_MAX;
		for (int faceIdx : vert.enclosing_faces)
		{
			const HullFace* face = &m_CubeHull->GetFace(faceIdx);
			float temp_correlation = maths::Vector3::Dot(local_axis, face->normal);
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
			maths::Vector3 wsPointOnPlane = wsTransform * m_CubeHull->GetVertex(m_CubeHull->GetEdge(best_face->edge_ids[0]).vStart).pos;
			maths::Vector3 planeNrml = -(normalMatrix * best_face->normal);
			planeNrml.Normalise();
			float planeDist = -maths::Vector3::Dot(planeNrml, wsPointOnPlane);

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
						planeDist = -maths::Vector3::Dot(planeNrml, wsPointOnPlane);

						out_adjacent_planes->emplace_back(planeNrml, planeDist);
					}
				}
			}
		}
	}

	void CuboidCollisionShape::DebugDraw(const PhysicsObject3D* currentObject) const
	{
		maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * maths::Matrix4::Scale(m_CuboidHalfDimensions);

		if (m_CubeHull->GetNumVertices() == 0)
		{
			ConstructCubeHull();
		}

		m_CubeHull->DebugDraw(transform);
	}

	void CuboidCollisionShape::ConstructCubeHull()
	{
		//Vertices
		m_CubeHull->AddVertex(maths::Vector3(-1.0f, -1.0f, -1.0f));	// 0
		m_CubeHull->AddVertex(maths::Vector3(-1.0f, 1.0f, -1.0f));		// 1
		m_CubeHull->AddVertex(maths::Vector3(1.0f, 1.0f, -1.0f));		// 2
		m_CubeHull->AddVertex(maths::Vector3(1.0f, -1.0f, -1.0f));		// 3

		m_CubeHull->AddVertex(maths::Vector3(-1.0f, -1.0f, 1.0f));		// 4
		m_CubeHull->AddVertex(maths::Vector3(-1.0f, 1.0f, 1.0f));		// 5
		m_CubeHull->AddVertex(maths::Vector3(1.0f, 1.0f, 1.0f));		// 6
		m_CubeHull->AddVertex(maths::Vector3(1.0f, -1.0f, 1.0f));		// 7

		int face1[] = { 0, 1, 2, 3 };
		int face2[] = { 7, 6, 5, 4 };
		int face3[] = { 5, 6, 2, 1 };
		int face4[] = { 0, 3, 7, 4 };
		int face5[] = { 6, 7, 3, 2 };
		int face6[] = { 4, 5, 1, 0 };

		//Faces
		m_CubeHull->AddFace(maths::Vector3(0.0f, 0.0f, -1.0f), 4, face1);
		m_CubeHull->AddFace(maths::Vector3(0.0f, 0.0f, 1.0f), 4, face2);
		m_CubeHull->AddFace(maths::Vector3(0.0f, 1.0f, 0.0f), 4, face3);
		m_CubeHull->AddFace(maths::Vector3(0.0f, -1.0f, 0.0f), 4, face4);
		m_CubeHull->AddFace(maths::Vector3(1.0f, 0.0f, 0.0f), 4, face5);
		m_CubeHull->AddFace(maths::Vector3(-1.0f, 0.0f, 0.0f), 4, face6);
	}
}
