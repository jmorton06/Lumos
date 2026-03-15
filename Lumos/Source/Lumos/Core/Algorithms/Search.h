#pragma once
#include "Core/Core.h"

namespace Lumos
{
    namespace Algorithms
    {
        template <typename Iterator, typename T, typename CompareFunc>
        Iterator LowerBound(Iterator first, Iterator last, const T& value, CompareFunc compare)
        {
            int count = (int)(last - first);
            while(count > 0)
            {
                int step      = count / 2;
                Iterator mid  = first + step;
                if(compare(*mid, value))
                {
                    first = mid + 1;
                    count -= step + 1;
                }
                else
                    count = step;
            }
            return first;
        }

        template <typename Iterator, typename T>
        Iterator LowerBound(Iterator first, Iterator last, const T& value)
        {
            return LowerBound(first, last, value, [](const T& a, const T& b) { return a < b; });
        }

        template <typename Iterator, typename T, typename CompareFunc>
        Iterator UpperBound(Iterator first, Iterator last, const T& value, CompareFunc compare)
        {
            int count = (int)(last - first);
            while(count > 0)
            {
                int step      = count / 2;
                Iterator mid  = first + step;
                if(!compare(value, *mid))
                {
                    first = mid + 1;
                    count -= step + 1;
                }
                else
                    count = step;
            }
            return first;
        }

        template <typename Iterator, typename T>
        Iterator UpperBound(Iterator first, Iterator last, const T& value)
        {
            return UpperBound(first, last, value, [](const T& a, const T& b) { return a < b; });
        }

        template <typename Iterator, typename T, typename CompareFunc>
        Iterator BinarySearch(Iterator first, Iterator last, const T& value, CompareFunc compare)
        {
            Iterator it = LowerBound(first, last, value, compare);
            if(it != last && !compare(value, *it) && !compare(*it, value))
                return it;
            return last;
        }

        template <typename Iterator, typename T>
        Iterator BinarySearch(Iterator first, Iterator last, const T& value)
        {
            return BinarySearch(first, last, value, [](const T& a, const T& b) { return a < b; });
        }
    }
}
