
#include "lmpch.h"
#include "SortAndSweepBroadphase.h"

namespace Lumos
{

    SortAndSweepBroadphase::SortAndSweepBroadphase(const Maths::Vector3 &axis)
		: Broadphase(), m_axisIndex(0)
	{
		SetAxis(axis);
	}

	SortAndSweepBroadphase::~SortAndSweepBroadphase()
	{
	}

	void SortAndSweepBroadphase::SetAxis(const Maths::Vector3 &axis)
	{
		// Determine axis
		m_axis = axis;
		m_axis.Normalize();

		if (abs(m_axis.x) > 0.9f)
			m_axisIndex = 0;
		else if (abs(m_axis.y) > 0.9f)
			m_axisIndex = 1;
		else if (abs(m_axis.z) > 0.9f)
			m_axisIndex = 2;
	}

	void SortAndSweepBroadphase::FindPotentialCollisionPairs(std::vector<Ref<PhysicsObject3D>>& objects,
	                                                         std::vector<CollisionPair> &collisionPairs)
	{
		// Sort entities along axis
		std::sort(objects.begin(), objects.end(), [this](Ref<PhysicsObject3D> a, Ref<PhysicsObject3D> b) -> bool
		{
			return a->GetWorldSpaceAABB().min_[this->m_axisIndex] < b->GetWorldSpaceAABB().min_[this->m_axisIndex];
		});

		for (auto it = objects.begin(); it != objects.end(); ++it)
		{
			float thisBoxRight = (*it)->GetWorldSpaceAABB().max_[m_axisIndex];

			for (auto iit = it + 1; iit != objects.end(); ++iit)
			{
				// Skip pairs of two at rest/static objects
				if (((*it)->GetIsAtRest() || (*it)->GetIsStatic()) && ((*iit)->GetIsAtRest() || (*iit)->GetIsStatic()))
					continue;

				float testBoxLeft = (*iit)->GetWorldSpaceAABB().min_[m_axisIndex];

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
	}
}
