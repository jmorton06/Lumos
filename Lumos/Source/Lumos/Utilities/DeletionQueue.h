#pragma once
#include <deque>
namespace Lumos
{
    struct DeletionQueue
    {
        std::deque<std::function<void()>> Deletors;

        template <typename F>
        void PushFunction(F&& function)
        {
            LUMOS_ASSERT(sizeof(F) < 200, "Lambda too large");
            Deletors.push_back(function);
        }

        void Flush()
        {
            for(auto it = Deletors.rbegin(); it != Deletors.rend(); it++)
            {
                (*it)();
            }

            Deletors.clear();
        }
    };
}