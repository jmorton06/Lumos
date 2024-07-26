#pragma once
#include "Core/Core.h"

namespace Lumos
{
    namespace Algorithms
    {
        template <typename T>
        struct SmallerThan
        {
            constexpr bool operator()(const T& a, const T& b) { return a < b; }
        };

        template <typename Iterator, typename CompareFunc = SmallerThan<typename RemovePointer<Iterator>::type>>
        constexpr void BubbleSort(Iterator first, Iterator last, CompareFunc compare = CompareFunc())
        {
            bool doSwap = true;
            while(doSwap)
            {
                doSwap      = false;
                Iterator p0 = first;
                Iterator p1 = first + 1;
                while(p1 != last)
                {
                    if(compare(*p1, *p0))
                    {
                        Swap(*p1, *p0);
                        doSwap = true;
                    }
                    ++p0;
                    ++p1;
                }
            }
        }
    }
}