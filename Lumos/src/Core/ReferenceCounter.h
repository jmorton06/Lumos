#pragma once


namespace Lumos
{
    // Atomic functions, these are used for multithread safe reference counters!
    
#ifdef NO_THREADS
    
    /* Bogus implementation unaware of multiprocessing */
    
    template <class T>
    static inline T atomic_conditional_increment(volatile T *pw)
    {
        if (*pw == 0)
            return 0;
        
        (*pw)++;
        
        return *pw;
    }
    
    template <class T>
    static inline T atomic_decrement(volatile T *pw)
    {
        (*pw)--;
        
        return *pw;
    }
    
    template <class T>
    static inline T atomic_increment(volatile T *pw)
    {
        (*pw)++;
        
        return *pw;
    }
    
    template <class T, class V>
    static _ALWAYS_INLINE_ T atomic_sub(volatile T *pw, volatile V val) {
        
        (*pw) -= val;
        
        return *pw;
    }
    
    template <class T, class V>
    static inline T atomic_add(volatile T *pw, volatile V val) {
        
        (*pw) += val;
        
        return *pw;
    }
    
    template <class T, class V>
    static inline T atomic_exchange_if_greater(volatile T *pw, volatile V val) {
        
        if (val > *pw)
            *pw = val;
        
        return *pw;
    }
    
#elif defined(__GNUC__)
    
    /* Implementation for GCC & Clang */
    
    // GCC guarantees atomic intrinsics for sizes of 1, 2, 4 and 8 bytes.
    // Clang states it supports GCC atomic builtins.
    
    template <class T>
    static inline T atomic_conditional_increment(volatile T *pw) {
        
        while (true) {
            T tmp = static_cast<T const volatile &>(*pw);
            if (tmp == 0)
                return 0; // if zero, can't add to it anymore
            if (__sync_val_compare_and_swap(pw, tmp, tmp + 1) == tmp)
                return tmp + 1;
        }
    }
    
    template <class T>
    static inline T atomic_decrement(volatile T *pw) {
        
        return __sync_sub_and_fetch(pw, 1);
    }
    
    template <class T>
    static inline T atomic_increment(volatile T *pw) {
        
        return __sync_add_and_fetch(pw, 1);
    }
    
    template <class T, class V>
    static inline T atomic_sub(volatile T *pw, volatile V val) {
        
        return __sync_sub_and_fetch(pw, val);
    }
    
    template <class T, class V>
    static inline T atomic_add(volatile T *pw, volatile V val) {
        
        return __sync_add_and_fetch(pw, val);
    }
    
    template <class T, class V>
    static inline T atomic_exchange_if_greater(volatile T *pw, volatile V val) {
        
        while (true) {
            T tmp = static_cast<T const volatile &>(*pw);
            if (tmp >= val)
                return tmp; // already greater, or equal
            if (__sync_val_compare_and_swap(pw, tmp, val) == tmp)
                return val;
        }
    }
    
#elif defined(_MSC_VER)
    // For MSVC use a separate compilation unit to prevent windows.h from polluting
    // the global namespace.
    uint32_t atomic_conditional_increment(volatile uint32_t *pw);
    uint32_t atomic_decrement(volatile uint32_t *pw);
    uint32_t atomic_increment(volatile uint32_t *pw);
    uint32_t atomic_sub(volatile uint32_t *pw, volatile uint32_t val);
    uint32_t atomic_add(volatile uint32_t *pw, volatile uint32_t val);
    uint32_t atomic_exchange_if_greater(volatile uint32_t *pw, volatile uint32_t val);
    
    uint64_t atomic_conditional_increment(volatile uint64_t *pw);
    uint64_t atomic_decrement(volatile uint64_t *pw);
    uint64_t atomic_increment(volatile uint64_t *pw);
    uint64_t atomic_sub(volatile uint64_t *pw, volatile uint64_t val);
    uint64_t atomic_add(volatile uint64_t *pw, volatile uint64_t val);
    uint64_t atomic_exchange_if_greater(volatile uint64_t *pw, volatile uint64_t val);
    
#else
    //no threads supported?
#error Must provide atomic functions for this platform or compiler!
#endif
    
    struct LUMOS_EXPORT ReferenceCounter
    {
        uint32_t count;
        
    public:
        // destroy() is called when weak_count_ drops to zero.
        
        inline bool ref()
        { //true on success
            return atomic_conditional_increment(&count) != 0;
        }
        
        inline uint32_t refval() { //true on success
            
            return atomic_conditional_increment(&count);
        }
        
        inline bool unref() { // true if must be disposed of
            
            return atomic_decrement(&count) == 0;
        }
        
        inline uint32_t get() const { // nothrow
            
            return count;
        }
        
        inline void init(uint32_t p_value = 1) {
            
            count = p_value;
        }
    };
}
