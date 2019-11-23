#pragma once
#include "lmpch.h"

#include "Maths/Maths.h"
#include "Physics/LumosPhysicsEngine/PhysicsObject3D.h"


namespace Lumos
{

	class PathEdge;

	class LUMOS_EXPORT PathNode : public PhysicsObject3D
	{
	public:
		explicit PathNode(const Maths::Vector3& position = Maths::Vector3());
		virtual ~PathNode();

		_FORCE_INLINE_ size_t NumConnections() const
		{
			return m_connections.size();
		}

		_FORCE_INLINE_ PathEdge *Edge(size_t i)
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
