#include "Precompiled.h"

#include "Maths/Maths.h"
#include "OctreeBroadphase.h"
#include "Graphics/Renderers/DebugRenderer.h"

namespace Lumos
{

    OctreeBroadphase::OctreeBroadphase(const size_t maxObjectsPerPartition, const size_t maxPartitionDepth)
        : m_MaxObjectsPerPartition(maxObjectsPerPartition)
        , m_MaxPartitionDepth(maxPartitionDepth)
        , m_Leaves()
    {
        m_NodePool[0].ChildCount         = 0;
        m_NodePool[0].PhysicsObjectCount = 0;
        m_NodePool[0].Index              = 0;
    }

    OctreeBroadphase::~OctreeBroadphase()
    {
    }

    void OctreeBroadphase::FindPotentialCollisionPairs(RigidBody3D* rootObject,
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

        RigidBody3D* current = rootObject;
        while(current)
        {
            if(current->GetCollisionShape())
            {
                LUMOS_PROFILE_SCOPE_LOW("Merge Bounding box and add Physics Object");
                rootNode.boundingBox.Merge(current->GetWorldSpaceAABB());
                rootNode.PhysicsObjects[rootNode.PhysicsObjectCount] = current;
                rootNode.PhysicsObjectCount++;
            }

            current = current->m_Next;
        }

        m_CurrentPoolIndex++;

        // Recursively divide world
        Divide(rootNode, 0);

        // Add collision pairs in leaf world divisions
        for(uint32_t leafIndex = 0; leafIndex < m_LeafCount; leafIndex++)
        {
            uint32_t objectCount = m_NodePool[m_Leaves[leafIndex]].PhysicsObjectCount;
            if(objectCount == 0)
                continue;

            for(size_t i = 0; i < objectCount - 1; ++i)
            {
                RigidBody3D& obj1 = *m_NodePool[m_Leaves[leafIndex]].PhysicsObjects[i];

                for(size_t j = i + 1; j < objectCount; ++j)
                {
                    RigidBody3D& obj2 = *m_NodePool[m_Leaves[leafIndex]].PhysicsObjects[j];

                    if(!obj1.GetCollisionShape() || !obj2.GetCollisionShape())
                        continue;

                    // Skip pairs of two at objects at rest
                    if(obj1.GetIsAtRest() && obj2.GetIsAtRest())
                        continue;

                    // Skip pairs of two at static objects
                    if(obj1.GetIsStatic() && obj2.GetIsStatic())
                        continue;

                    // Skip pairs of one static and one at rest
                    if(obj1.GetIsAtRest() && obj2.GetIsStatic())
                        continue;

                    if(obj1.GetIsStatic() && obj2.GetIsAtRest())
                        continue;

                    CollisionPair pair;

                    if(&obj1 < &obj2)
                    {
                        pair.pObjectA = &obj1;
                        pair.pObjectB = &obj2;
                    }
                    else
                    {
                        pair.pObjectA = &obj2;
                        pair.pObjectB = &obj1;
                    }

                    bool duplicate = false;
                    for(int i = 0; i < collisionPairs.size(); i++)
                    {
                        auto& pair2 = collisionPairs[i];

                        if(pair.pObjectA == pair2.pObjectA && pair.pObjectB == pair2.pObjectB)
                        {
                            duplicate = true;
                        }
                    }
                    if(!duplicate)
                        collisionPairs.push_back(pair);
                }
            }
        }
    }

    void OctreeBroadphase::DebugDraw()
    {
        DebugDrawOctreeNode(m_NodePool[0]);
    }

    void OctreeBroadphase::Divide(OctreeNode& division, const size_t iteration)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        // Exit conditions (partition depth limit or target object count reached)
        if(iteration > m_MaxPartitionDepth || division.PhysicsObjectCount <= m_MaxObjectsPerPartition)
        {
            LUMOS_PROFILE_SCOPE_LOW("Add Leaf");
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
            LUMOS_PROFILE_SCOPE_LOW("Create Child");

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
                LUMOS_PROFILE_SCOPE_LOW("PhysicsObject BB check");
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
