#pragma once

#include "ReferenceCounter.h"
#include <memory>

namespace Lumos
{
    class ReferenceBase
    {
    public:
        ReferenceBase();
        ~ReferenceBase();
        
        inline bool IsReferenced() const { return m_RefcountInit.get() < 1; }
        bool InitRef();
        bool reference(); // returns false if refcount is at zero and didn't get increased
        bool unreference();
        int GetReferenceCount() const;
    protected:
        static void _bind_methods();
    private:
        ReferenceCounter m_Refcount;
        ReferenceCounter m_RefcountInit;
    };
    
    template<class T>
    class Reference
    {
        
    private:
        T* m_Ptr;
    };
    
    template<class T>
    class WeakReference
    {
        
    private:
        T* m_Ptr;
    };
    
    template<class T>
    class Owned
    {
        
    private:
        T* m_Ptr;

    };
    
    template<class T>
    using Ref = std::shared_ptr<T>;
    
    template <typename T, typename ... Args>
    Ref<T> CreateRef(Args&& ...args)
    {
        return std::make_shared<T>(args ...);
    }
    
    template <typename T>
    Ref<T> CreateRef(T* t)
    {
        return std::shared_ptr<T>(t);
    }
}
