#pragma once
#include "Core.h"
#include <stdint.h>
#include <atomic>

namespace Lumos
{
    struct LUMOS_EXPORT ReferenceCounter
    {
        std::atomic<int> count = 0;

    public:
        inline bool SharedPtr()
        {
            return count.fetch_add(1, std::memory_order_relaxed) >= 0;
        }

        inline int RefValue()
        {
            return count.fetch_add(1, std::memory_order_relaxed) + 1;
        }

        // Returns true when this call dropped the count to zero.
        // Uses acq_rel so the destructor that follows sees all writes made
        // by other threads while they held a reference.
        inline bool Unref()
        {
            return count.fetch_sub(1, std::memory_order_acq_rel) == 1;
        }

        // CAS-loop: increment only if the current value is greater than zero.
        // Used by WeakReference::Lock to safely promote without racing the
        // last strong unref.
        inline bool TryRef()
        {
            int curr = count.load(std::memory_order_relaxed);
            while(curr > 0)
            {
                if(count.compare_exchange_weak(curr, curr + 1,
                                               std::memory_order_acquire,
                                               std::memory_order_relaxed))
                    return true;
            }
            return false;
        }

        inline int Get() const
        {
            return count.load(std::memory_order_relaxed);
        }

        inline void Init(int p_value = 1)
        {
            count.store(p_value, std::memory_order_relaxed);
        }
    };
}
