#pragma once
#include "Core/Core.h"
#include "Find.h"

namespace Lumos
{
    namespace Algorithms
    {
        template <typename T>
        struct EqualTo
        {
            constexpr bool operator()(const T& a, const T& b) { return a == b; }
        };

        template <typename ForwardIterator, typename CompareFunc = EqualTo<typename RemovePointer<ForwardIterator>::type>>
        ForwardIterator RemoveIf(ForwardIterator first, ForwardIterator last, CompareFunc&& compare = CompareFunc())
        {
            auto found = FindIf(first, last, compare);
            if(found != last)
            {
                auto it = found;
                while(++it != last)
                {
                    if(!predicate(*it))
                    {
                        *found++ = Move(*it);
                    }
                }
            }
            return found;
        }
    }
}