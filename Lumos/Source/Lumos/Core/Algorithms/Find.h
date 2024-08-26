#pragma once
#include "Core/Core.h"

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
        constexpr ForwardIterator FindIf(ForwardIterator first, ForwardIterator last, CompareFunc&& value)
        {
            for(auto it = first; it != last; ++it)
            {
                if(value == (*it))
                {
                    return it;
                }
            }
            return last;
        }
    }
}