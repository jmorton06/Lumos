
#include "JM.h"
#include "SortAndSweepBroadphase.h"

#include "Graphics/Renderers/DebugRenderer.h"
#include <memory>
#include <vector>

namespace jm
{

	SortAndSweepBroadphase::SortAndSweepBroadphase(const maths::Vector3 &axis)
		: Broadphase()
	{
		SetAxis(axis);
	}

	SortAndSweepBroadphase::~SortAndSweepBroadphase()
	{
	}

	void SortAndSweepBroadphase::SetAxis(const maths::Vector3 &axis)
	{
		// Determine axis
		m_axis = axis;
		m_axis.Normalise();

		if (abs(m_axis.GetX()) > 0.9f)
			m_axisIndex = 0;
		else if (abs(m_axis.GetY()) > 0.9f)
			m_axisIndex = 1;
		else if (abs(m_axis.GetZ()) > 0.9f)
			m_axisIndex = 2;
	}

	void SortAndSweepBroadphase::FindPotentialCollisionPairs(std::vector<std::shared_ptr<PhysicsObject3D>> objects,
	                                                         std::vector<CollisionPair> &collisionPairs)
	{
		// Sort entities along axis
		std::sort(objects.begin(), objects.end(), [this](std::shared_ptr<PhysicsObject3D> a, std::shared_ptr<PhysicsObject3D> b) -> bool
		{
			return a->GetWorldSpaceAABB().Lower()[this->m_axisIndex] < b->GetWorldSpaceAABB().Lower()[this->m_axisIndex];
		});

		for (auto it = objects.begin(); it != objects.end(); ++it)
		{
			float thisBoxRight = (*it)->GetWorldSpaceAABB().Upper()[m_axisIndex];

			for (auto iit = it + 1; iit != objects.end(); ++iit)
			{
				// Skip pairs of two at rest objects
				if ((*it)->GetIsAtRest() && (*iit)->GetIsAtRest())
					continue;

				// Skip pairs of two at static objects
				if ((*it)->GetIsStatic() && (*iit)->GetIsStatic())
					continue;

				// Skip pairs of one static and one at rest
				if ((*it)->GetIsAtRest() && (*iit)->GetIsStatic())
					continue;

				if ((*it)->GetIsStatic() && (*iit)->GetIsAtRest())
					continue;

				float testBoxLeft = (*iit)->GetWorldSpaceAABB().Lower()[m_axisIndex];

				// Test for overlap between the axis values of the bounding boxes
				if (testBoxLeft < thisBoxRight)
				{
					CollisionPair cp;
					cp.pObjectA = (*it).get();
					cp.pObjectB = (*iit).get();

					collisionPairs.push_back(cp);
				}
			}
		}
	}

	void SortAndSweepBroadphase::DebugDraw()
	{
		DebugRenderer::DrawPointNDT(maths::Vector3(0.0f, 0.0f, 0.0f), 0.05f, maths::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
		DebugRenderer::DrawThickLine(maths::Vector3(0.0f, 0.0f, 0.0f), m_axis, 0.02f, maths::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
	}
}
