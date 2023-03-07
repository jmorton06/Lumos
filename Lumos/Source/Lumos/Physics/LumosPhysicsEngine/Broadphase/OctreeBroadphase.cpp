#include "Precompiled.h"

#include "Maths/Maths.h"
#include "OctreeBroadphase.h"
#include "Graphics/Renderers/DebugRenderer.h"

namespace Lumos
{

    OctreeBroadphase::OctreeBroadphase(const size_t maxObjectsPerPartition, const size_t maxPartitionDepth, const SharedPtr<Broadphase>& secondaryBroadphase)
        : m_MaxObjectsPerPartition(maxObjectsPerPartition)
        , m_MaxPartitionDepth(maxPartitionDepth)
        , m_SecondaryBroadphase(secondaryBroadphase)
        , m_Leaves()
    {
        m_NodePool[0].ChildCount         = 0;
        m_NodePool[0].PhysicsObjectCount = 0;
        m_NodePool[0].Index              = 0;
    }

    OctreeBroadphase::~OctreeBroadphase()
    {
    }

    void OctreeBroadphase::FindPotentialCollisionPairs(RigidBody3D** objects, uint32_t objectCount,
                                                       std::vector<CollisionPair>& collisionPairs)
    {
        LUMOS_PROFILE_FUNCTION();
        m_CurrentPoolIndex = 0;
        m_LeafCount        = 0;

        auto& rootNode              = m_NodePool[0];
        rootNode.ChildCount         = 0;
        rootNode.PhysicsObjectCount = 0;
        rootNode.Index              = 0;
        rootNode.boundingBox        = Maths::BoundingBox();

        for(uint32_t i = 0; i < objectCount; i++)
        {
            auto physicsObject = objects[i];

            if(physicsObject && physicsObject->GetCollisionShape())
            {
                LUMOS_PROFILE_SCOPE("Merge Bounding box and add Physics Object");
                rootNode.boundingBox.Merge(physicsObject->GetWorldSpaceAABB());
                rootNode.PhysicsObjects[rootNode.PhysicsObjectCount] = physicsObject;
                rootNode.PhysicsObjectCount++;
            }
        }

        m_CurrentPoolIndex++;

        // Recursively divide world
        Divide(rootNode, 0);

        // Add collision pairs in leaf world divisions
        for(uint32_t i = 0; i < m_LeafCount; i++)
        {
            if(m_NodePool[m_Leaves[i]].PhysicsObjectCount > 1)
                m_SecondaryBroadphase->FindPotentialCollisionPairs(m_NodePool[m_Leaves[i]].PhysicsObjects, m_NodePool[m_Leaves[i]].PhysicsObjectCount, collisionPairs);
        }
    }

    void OctreeBroadphase::DebugDraw()
    {
        DebugDrawOctreeNode(m_NodePool[0]);
    }

    void OctreeBroadphase::Divide(OctreeNode& division, const size_t iteration)
    {
        LUMOS_PROFILE_FUNCTION();
        // Exit conditions (partition depth limit or target object count reached)
        if(iteration > m_MaxPartitionDepth || division.PhysicsObjectCount <= m_MaxObjectsPerPartition)
        {
            LUMOS_PROFILE_SCOPE("Add Leaf");
            // Ignore any subdivisions that contain no objects
            if(division.PhysicsObjectCount != 0)
            {
                m_Leaves[m_LeafCount] = division.Index;
                m_LeafCount++;
            }

            return;
        }

        glm::vec3 divisionPoints[3] = { division.boundingBox.Min(), division.boundingBox.Center(), division.boundingBox.Max() };

        static const size_t NUM_DIVISIONS                            = 8;
        static const size_t DIVISION_POINT_INDICES[NUM_DIVISIONS][6] = {
            { 0, 0, 0, 1, 1, 1 },
            { 1, 0, 0, 2, 1, 1 },
            { 0, 1, 0, 1, 2, 1 },
            { 1, 1, 0, 2, 2, 1 },
            { 0, 0, 1, 1, 1, 2 },
            { 1, 0, 1, 2, 1, 2 },
            { 0, 1, 1, 1, 2, 2 },
            { 1, 1, 1, 2, 2, 2 }
        };

        for(size_t i = 0; i < NUM_DIVISIONS; i++)
        {
            LUMOS_PROFILE_SCOPE("Create Child");

            division.ChildNodeIndices[division.ChildCount] = m_CurrentPoolIndex;
            auto& newNode                                  = m_NodePool[m_CurrentPoolIndex];
            newNode.ChildCount                             = 0;
            newNode.PhysicsObjectCount                     = 0;
            newNode.Index                                  = m_CurrentPoolIndex;

            division.ChildCount++;
            m_CurrentPoolIndex++;

            const glm::vec3 lower(divisionPoints[DIVISION_POINT_INDICES[i][0]].x,
                                  divisionPoints[DIVISION_POINT_INDICES[i][1]].y,
                                  divisionPoints[DIVISION_POINT_INDICES[i][2]].z);
            const glm::vec3 upper(divisionPoints[DIVISION_POINT_INDICES[i][3]].x,
                                  divisionPoints[DIVISION_POINT_INDICES[i][4]].y,
                                  divisionPoints[DIVISION_POINT_INDICES[i][5]].z);

            newNode.boundingBox.m_Min = lower;
            newNode.boundingBox.m_Max = upper;

            // Add objects inside division
            for(uint32_t i = 0; i < division.PhysicsObjectCount; i++)
            {
                LUMOS_PROFILE_SCOPE("PhysicsObject BB check");
                RigidBody3D* physicsObject = division.PhysicsObjects[i];

                if(!physicsObject)
                    continue;

                const Maths::BoundingBox& boundingBox = physicsObject->GetWorldSpaceAABB();
                Intersection intersection             = newNode.boundingBox.IsInside(boundingBox);
                if(intersection != OUTSIDE)
                {
                    newNode.PhysicsObjects[newNode.PhysicsObjectCount] = physicsObject;
                    newNode.PhysicsObjectCount++;

                    if(intersection == INSIDE)
                        division.PhysicsObjects[i] = nullptr;
                }
            }

            // Do further subdivisioning
            Divide(newNode, iteration + 1);
        }
    }

    void OctreeBroadphase::DebugDrawOctreeNode(const OctreeNode& node)
    {
        DebugRenderer::DebugDraw(node.boundingBox, glm::vec4(0.8f, 0.2f, 0.4f, 1.0f), false, 0.1f);

        // Draw sub divisions
        for(uint32_t i = 0; i < node.ChildCount; i++)
            DebugDrawOctreeNode(m_NodePool[node.ChildNodeIndices[i]]);
    }
}
