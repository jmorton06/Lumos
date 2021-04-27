#pragma once
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Maths/Vector3.h"
#include <vector>

namespace Lumos
{

    class PathEdge;

    class PathNode : public RigidBody3D
    {
    public:
        explicit PathNode(const Maths::Vector3& position = Maths::Vector3());
        virtual ~PathNode();

        size_t NumConnections() const
        {
            return m_connections.size();
        }

        PathEdge* Edge(size_t i)
        {
            return m_connections[i];
        }

        bool IsOnList(const std::vector<PathNode*>& list);

        virtual float HeuristicValue(const PathNode& other) const;

    private:
        friend class PathEdge;
        std::vector<PathEdge*> m_connections;
    };

}
