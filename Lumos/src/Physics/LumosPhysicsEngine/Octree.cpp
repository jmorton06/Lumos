#include "LM.h"
#include "Maths/BoundingBox.h"
#include "Octree.h"

namespace Lumos
{

	Octree::Octree(const size_t maxObjectsPerPartition, const size_t maxPartitionDepth,const Ref<Broadphase>& secondaryBroadphase)
		: m_MaxObjectsPerPartition(maxObjectsPerPartition)
		, m_MaxPartitionDepth(maxPartitionDepth)
		, m_SecondaryBroadphase(secondaryBroadphase)
		, m_RootNode(nullptr)
	{
	}

	Octree::~Octree()
	{
		m_LeafNodes.clear();
	}

	void Octree::FindPotentialCollisionPairs(std::vector<Ref<PhysicsObject3D>>& objects,
	                                         std::vector<CollisionPair>& collisionPairs)
	{
		m_LeafNodes.clear();

		m_LeafNodes.reserve(m_MaxPartitionDepth * m_MaxPartitionDepth * m_MaxPartitionDepth);

		// Init root world space
		m_RootNode.reset();
		m_RootNode = CreateRef<OctreeNode>();
		m_RootNode->physicsObjects.reserve(m_MaxObjectsPerPartition);
		m_RootNode->childNodes.reserve(8);

		for (auto& physicsObject : objects)
		{	 
            if (physicsObject && physicsObject->GetCollisionShape())
			{
				m_RootNode->boundingBox.ExpandToFit(physicsObject->GetWorldSpaceAABB());
				m_RootNode->physicsObjects.push_back(physicsObject);
			}
		}

		// Recursively divide world
		Divide(m_RootNode, 0);

		// Add collision pairs in leaf world divisions
		for (auto &m_LeafNode : m_LeafNodes)
			m_SecondaryBroadphase->FindPotentialCollisionPairs(m_LeafNode->physicsObjects, collisionPairs);
	}

	void Octree::DebugDraw()
	{
		DebugDrawOctreeNode(m_RootNode.get());
	}

	void Octree::Divide(const Ref<OctreeNode>& division, const size_t iteration)
	{
		// Exit conditions (partition depth limit or target object count reached)
		if (iteration > m_MaxPartitionDepth || division->physicsObjects.size() <= m_MaxObjectsPerPartition)
		{
			// Ignore any subdivisions that contain no objects
			if (!division->physicsObjects.empty())
				m_LeafNodes.push_back(division);

			return;
		}

		Maths::Vector3 divisionPoints[] = { division->boundingBox.Lower(), division->boundingBox.Centre(), division->boundingBox.Upper() };

		static const size_t NUM_DIVISIONS = 8;
		static const size_t DIVISION_POINT_INDICES[NUM_DIVISIONS][6] = 
		{
			{ 0, 0, 0, 1, 1, 1 },
			{ 1, 0, 0, 2, 1, 1 },
			{ 0, 1, 0, 1, 2, 1 },
			{ 1, 1, 0, 2, 2, 1 },
			{ 0, 0, 1, 1, 1, 2 },
			{ 1, 0, 1, 2, 1, 2 },
			{ 0, 1, 1, 1, 2, 2 },
			{ 1, 1, 1, 2, 2, 2 }
		};

		for (size_t i = 0; i < NUM_DIVISIONS; i++)
		{
			Ref<OctreeNode> newNode = CreateRef<OctreeNode>();
			newNode->physicsObjects.reserve(m_MaxObjectsPerPartition);
			newNode->childNodes.reserve(8);

			const Maths::Vector3 lower(divisionPoints[DIVISION_POINT_INDICES[i][0]].GetX(),
				divisionPoints[DIVISION_POINT_INDICES[i][1]].GetY(),
				divisionPoints[DIVISION_POINT_INDICES[i][2]].GetZ());
			const Maths::Vector3 upper(divisionPoints[DIVISION_POINT_INDICES[i][3]].GetX(),
				divisionPoints[DIVISION_POINT_INDICES[i][4]].GetY(),
				divisionPoints[DIVISION_POINT_INDICES[i][5]].GetZ());

			newNode->boundingBox = Maths::BoundingBox(lower, upper);

			// Add objects inside division
			for (auto &physicsObject : division->physicsObjects)
			{
				if (newNode->boundingBox.Intersects(physicsObject->GetWorldSpaceAABB()))
					newNode->physicsObjects.push_back(physicsObject);
			}

			// Add to parent division
			division->childNodes.emplace_back(newNode);

			// Do further subdivisioning
			Divide(newNode, iteration + 1);
		}
	}

	void Octree::DebugDrawOctreeNode(OctreeNode* node)
	{
	}
}
