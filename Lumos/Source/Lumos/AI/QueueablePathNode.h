#pragma once

#include "PathNode.h"
#include <limits>

#undef max

namespace Lumos
{

    class LUMOS_EXPORT QueueablePathNode
    {
    public:
        explicit QueueablePathNode(PathNode* n)
            : node(n)
            , Parent(nullptr)
            , fScore(std::numeric_limits<float>::max())
            , gScore(std::numeric_limits<float>::max())
        {
        }

        virtual ~QueueablePathNode()
        {
        }

        bool IsOnList(const std::vector<QueueablePathNode*>& list)
        {
            return std::find_if(list.begin(), list.end(), [this](QueueablePathNode* n)
                       { return n->node == reinterpret_cast<PathNode*>(this); })
                != list.end();
        }

        bool operator>(const QueueablePathNode& other) const
        {
            return this->fScore > other.fScore;
        }

        bool operator>=(const QueueablePathNode& other) const
        {
            return this->fScore >= other.fScore;
        }

        bool operator<(const QueueablePathNode& other) const
        {
            return this->fScore < other.fScore;
        }

        bool operator<=(const QueueablePathNode& other) const
        {
            return this->fScore <= other.fScore;
        }

    public:
        PathNode* node; //!< Wrapped Node
        QueueablePathNode* Parent; //!< Parent Node in path
        float fScore; //!< F score of wrapped node
        float gScore; //!< G score of wrapped node
    };

}