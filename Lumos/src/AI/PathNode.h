#pragma once
#include "LM.h"

#include "Maths/Maths.h"
#include "Physics/LumosPhysicsEngine/PhysicsObject3D.h"


namespace Lumos
{

	class PathEdge;

	class LUMOS_EXPORT PathNode : public PhysicsObject3D
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