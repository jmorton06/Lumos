#pragma once
#include "Core/DataStructures/Vector.h"
namespace Lumos
{
    struct DeletionQueue
    {
        Vector<std::function<void()>> Deletors;

        template <typename F>
        void PushFunction(F&& function)
        {
            LUMOS_ASSERT(sizeof(F) < 200, "Lambda too large");
            Deletors.PushBack(function);
        }

        void Flush()
        {
            for(auto& deleteFunc : Deletors)
            {
                deleteFunc();
            }

            Deletors.Clear();
        }
    };
}