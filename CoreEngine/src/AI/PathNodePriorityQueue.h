#pragma once
#include "JM.h"
#include "QueueablePathNode.h"

namespace jm
{

	template <typename T> struct greater_ptr : std::binary_function<bool, const T *, const T *>
	{
		bool operator()(const T *a, const T *b) const
		{
			if (a == nullptr)
				// If b is also 0, then they are equal, hence a is not < than b
				return b != nullptr;
			else if (b == nullptr)
				return false;
			else
				return (*a) > (*b);
		}
	};

	class PathNodePriorityQueue : public std::vector<QueueablePathNode *>
	{
	public:
		PathNodePriorityQueue()
			: std::vector<QueueablePathNode *>()
			, m_comp()
		{
			std::make_heap(begin(), end(), m_comp);
		}

		void Push(QueueablePathNode *item)
		{
			push_back(item);
			std::push_heap(begin(), end(), m_comp);
		}

		void Pop()
		{
			std::pop_heap(begin(), end(), m_comp);
			pop_back();
		}

		QueueablePathNode *Top() const
		{
			return front();
		}

		std::vector<QueueablePathNode *>::const_iterator Find(QueueablePathNode *item) const
		{
			return std::find(cbegin(), cend(), item);
		}

		void Update()
		{
			std::make_heap(begin(), end(), m_comp);
		}

	private:
		greater_ptr<QueueablePathNode> m_comp;
	};

}