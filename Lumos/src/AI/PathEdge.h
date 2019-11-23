#pragma once
#include "lmpch.h"
#include "PathNode.h"

namespace Lumos
{

	class LUMOS_EXPORT PathEdge
	{
	public:
		PathEdge(PathNode* a, PathNode* b);
		virtual ~PathEdge();

		_FORCE_INLINE_ PathNode* NodeA() const
		{
			return m_NodeA;
		}

		_FORCE_INLINE_ PathNode* NodeB() const
		{
			return m_NodeB;
		}

		PathNode *OtherNode(PathNode *node);

		_FORCE_INLINE_ bool Traversable() const
		{
			return m_Traversable;
		}

		virtual float StaticCost() const;

		_FORCE_INLINE_ float Weight() const
		{
			return m_Weight;
		}

		_FORCE_INLINE_ float Cost() const
		{
			return StaticCost() * m_Weight;
		}

		void SetTraversable(bool traversable);
		void SetWeight(float weight);

		bool IsOnPath(const std::vector<PathNode *> &path) const
		{
			auto aIt = std::find(path.begin(), path.end(), m_NodeA);
			auto bIt = std::find(path.begin(), path.end(), m_NodeB);
			return (aIt != path.end() && bIt != path.end() && std::abs(std::distance(aIt, bIt)) == 1);
		}

		bool operator==(const PathEdge &other) const;
		bool operator!=(const PathEdge &other) const;

	private:
		PathNode* m_NodeA;
		PathNode* m_NodeB;

	protected:
		bool m_Traversable;
		float m_Weight;
	};

}
