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

        template <typename Iterator, typename CompareFunc = SmallerThan<typename RemovePointer<Iterator>::type>>
        constexpr void InsertionSort(Iterator first, Iterator last, CompareFunc compare = CompareFunc())
        {
            if(first == last)
                return;
            for(Iterator i = first + 1; i != last; ++i)
            {
                auto key = *i;
                Iterator j = i;
                while(j != first && compare(key, *(j + (-1))))
                {
                    *j = *(j + (-1));
                    j = j + (-1);
                }
                *j = key;
            }
        }

        namespace Detail
        {
            template <typename Iterator, typename CompareFunc>
            void SiftDown(Iterator first, int start, int end, CompareFunc& compare)
            {
                int root = start;
                while(root * 2 + 1 <= end)
                {
                    int child = root * 2 + 1;
                    if(child + 1 <= end && compare(*(first + child), *(first + child + 1)))
                        ++child;
                    if(compare(*(first + root), *(first + child)))
                    {
                        Swap(*(first + root), *(first + child));
                        root = child;
                    }
                    else
                        return;
                }
            }

            template <typename Iterator, typename CompareFunc>
            Iterator MedianOfThree(Iterator a, Iterator b, Iterator c, CompareFunc& compare)
            {
                if(compare(*a, *b))
                {
                    if(compare(*b, *c))
                        return b;
                    else if(compare(*a, *c))
                        return c;
                    else
                        return a;
                }
                else
                {
                    if(compare(*a, *c))
                        return a;
                    else if(compare(*b, *c))
                        return c;
                    else
                        return b;
                }
            }

            inline int FloorLog2(int n)
            {
                int log = 0;
                while(n > 1)
                {
                    n >>= 1;
                    ++log;
                }
                return log;
            }
        }

        template <typename Iterator, typename CompareFunc = SmallerThan<typename RemovePointer<Iterator>::type>>
        void HeapSort(Iterator first, Iterator last, CompareFunc compare = CompareFunc())
        {
            int count = (int)(last - first);
            if(count < 2)
                return;

            for(int start = (count - 2) / 2; start >= 0; --start)
                Detail::SiftDown(first, start, count - 1, compare);

            for(int end = count - 1; end > 0; --end)
            {
                Swap(*first, *(first + end));
                Detail::SiftDown(first, 0, end - 1, compare);
            }
        }

        template <typename Iterator, typename CompareFunc>
        void IntroSortImpl(Iterator first, Iterator last, int depthLimit, CompareFunc& compare)
        {
            while(last - first > 16)
            {
                if(depthLimit == 0)
                {
                    HeapSort(first, last, compare);
                    return;
                }
                --depthLimit;

                Iterator mid   = first + (last - first) / 2;
                Iterator pivot = Detail::MedianOfThree(first, mid, last + (-1), compare);
                Swap(*pivot, *(last + (-1)));

                Iterator i = first;
                Iterator j = last + (-1);
                for(;;)
                {
                    while(compare(*i, *(last + (-1))))
                        i = i + 1;
                    while(j != first && compare(*(last + (-1)), *(j + (-1))))
                        j = j + (-1);
                    if(!(i - j < 0))
                        break;
                    Swap(*i, *(j + (-1)));
                    i = i + 1;
                }
                Swap(*i, *(last + (-1)));

                IntroSortImpl(i + 1, last, depthLimit, compare);
                last = i;
            }
            InsertionSort(first, last, compare);
        }

        template <typename Iterator, typename CompareFunc = SmallerThan<typename RemovePointer<Iterator>::type>>
        void IntroSort(Iterator first, Iterator last, CompareFunc compare = CompareFunc())
        {
            int count = (int)(last - first);
            if(count < 2)
                return;
            int depthLimit = 2 * Detail::FloorLog2(count);
            IntroSortImpl(first, last, depthLimit, compare);
        }
    }
}