
#include "Precompiled.h"
#include "SortAndSweepBroadphase.h"

namespace Lumos
{

    SortAndSweepBroadphase::SortAndSweepBroadphase(const Maths::Vector3& axis)
        : Broadphase()
        , m_AxisIndex(0)
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
        m_Axis = axis;
        m_Axis.Normalise();

        if(abs(m_Axis.x) > 0.9f)
            m_AxisIndex = 0;
        else if(abs(m_Axis.y) > 0.9f)
            m_AxisIndex = 1;
        else if(abs(m_Axis.z) > 0.9f)
            m_AxisIndex = 2;
    }

    void SortAndSweepBroadphase::FindPotentialCollisionPairs(RigidBody3D** objects, uint32_t objectCount,
        std::vector<CollisionPair>& collisionPairs)
    {
        LUMOS_PROFILE_FUNCTION();
        // Sort entities along axis
        std::sort(objects, objects + objectCount, [this](RigidBody3D* a, RigidBody3D* b) -> bool
            { return a->GetWorldSpaceAABB().min_[this->m_AxisIndex] < b->GetWorldSpaceAABB().min_[this->m_AxisIndex]; });

        for(uint32_t i = 0; i < objectCount; i++)
        {
            auto& obj = *objects[i];

            float thisBoxRight = obj.GetWorldSpaceAABB().max_[m_AxisIndex];

            for(uint32_t iit = i + 1; iit < objectCount; iit++)
            {
                auto& obj2 = *objects[iit];
                // Skip pairs of two at rest/static objects
                if((obj.GetIsAtRest() || obj.GetIsStatic()) && (obj2.GetIsAtRest() || obj2.GetIsStatic()))
                    continue;

                float testBoxLeft = obj2.GetWorldSpaceAABB().min_[m_AxisIndex];

                // Test for overlap between the axis values of the bounding boxes
                if(testBoxLeft < thisBoxRight)
                {
                    CollisionPair cp;
                    cp.pObjectA = &obj;
                    cp.pObjectB = &obj2;

                    collisionPairs.push_back(cp);
                }
            }
        }
    }

    void SortAndSweepBroadphase::DebugDraw()
    {
    }
}
