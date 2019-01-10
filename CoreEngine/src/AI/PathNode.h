#pragma once
#include "JM.h"

#include "Maths/Maths.h"
#include "Physics/JMPhysicsEngine/PhysicsObject3D.h"


namespace jm
{

	class PathEdge;

	class CORE_DLL PathNode : public PhysicsObject3D
	{
	public:
		explicit PathNode(const maths::Vector3& position = maths::Vector3());
		virtual ~PathNode();

		inline size_t NumConnections() const
		{
			return m_connections.size();
		}

		inline PathEdge *Edge(size_t i)
		{
			return m_connections[i];
		}

		bool IsOnList(const std::vector<PathNode *> &list);

		virtual float HeuristicValue(const PathNode &other) const;

	private:
		friend class PathEdge;

		std::vector<PathEdge *> m_connections;
	};

}