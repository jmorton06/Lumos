
#include "Precompiled.h"
#include "SortAndSweepBroadphase.h"

namespace Lumos
{

	SortAndSweepBroadphase::SortAndSweepBroadphase(const Maths::Vector3& axis)
		: Broadphase()
		, m_axisIndex(0)
	{
		SetAxis(axis);
	}

	SortAndSweepBroadphase::~SortAndSweepBroadphase()
	{
	}

	void SortAndSweepBroadphase::SetAxis(const Maths::Vector3& axis)
	{
        LUMOS_PROFILE_FUNCTION();
		// Determine axis
		m_axis = axis;
		m_axis.Normalize();

		if(abs(m_axis.x) > 0.9f)
			m_axisIndex = 0;
		else if(abs(m_axis.y) > 0.9f)
			m_axisIndex = 1;
		else if(abs(m_axis.z) > 0.9f)
			m_axisIndex = 2;
	}

	void SortAndSweepBroadphase::FindPotentialCollisionPairs(Ref<RigidBody3D>* objects, u32 objectCount,
		std::vector<CollisionPair>& collisionPairs)
	{
        LUMOS_PROFILE_FUNCTION();
		// Sort entities along axis
		std::sort(objects, objects + objectCount, [this](Ref<RigidBody3D> a, Ref<RigidBody3D> b) -> bool {
			return a->GetWorldSpaceAABB().min_[this->m_axisIndex] < b->GetWorldSpaceAABB().min_[this->m_axisIndex];
		});

		for(u32 i = 0; i < objectCount; i++)
		{
			auto obj = objects[i];
			
			float thisBoxRight = obj->GetWorldSpaceAABB().max_[m_axisIndex];

            for(u32 iit = i + 1; iit < objectCount; iit++)
			{
                auto obj2 = objects[iit];
				// Skip pairs of two at rest/static objects
				if((obj->GetIsAtRest() || obj->GetIsStatic()) && (obj2->GetIsAtRest() || obj2->GetIsStatic()))
					continue;

				float testBoxLeft = obj2->GetWorldSpaceAABB().min_[m_axisIndex];

				// Test for overlap between the axis values of the bounding boxes
				if(testBoxLeft < thisBoxRight)
				{
					CollisionPair cp;
					cp.pObjectA = obj.get();
					cp.pObjectB = obj2.get();

					collisionPairs.push_back(cp);
				}
			}
		}
	}

	void SortAndSweepBroadphase::DebugDraw()
	{
	}
}
