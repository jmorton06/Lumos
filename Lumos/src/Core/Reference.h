#pragma once

#include "LM.h"
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
    
    template<class T, typename ...Args>
    class Reference
    {
    public:
        static Reference<T> CreateRef(Args&& ...args)
        {
            auto ptr = new T(args ...);
            
            return Reference<T>(ptr);
        }
        
        Reference()
        {
            m_Ptr = nullptr;
            m_Test = new ReferenceBase();
            m_Test->InitRef();
        }
        
        Reference(T* ptr)
        {
            m_Ptr = ptr;
            m_Test = new ReferenceBase();
            m_Test->InitRef();
        }
        
        Reference(const Reference& other)
        {
            m_Ptr = other.m_Ptr;
            m_Test = other.m_Test;
            m_Test->reference();
        }
        
        ~Reference()
        {
            if(m_Test->unreference())
            {
                delete m_Test;
                delete m_Ptr;
            }
        }
        
        inline T* get() const
        {
            return m_Ptr;
        }
        
        inline bool operator==(const T *p_ptr) const
        {
            return m_Ptr == p_ptr;
        }
        
        inline bool operator!=(const T *p_ptr) const
        {
            return m_Ptr != p_ptr;
        }
        
        inline bool operator<(const Reference<T> &p_r) const
        {
            return m_Ptr < p_r.m_Ptr;
        }
            
        inline bool operator==(const Reference<T> &p_r) const
        {
            return m_Ptr == p_r.m_Ptr;
        }
            
        inline bool operator!=(const Reference<T> &p_r) const
        {
            return m_Ptr != p_r.m_Ptr;
        }
            
        inline T *operator->()
        {
            return m_Ptr;
        }
            
        inline T *operator*()
        {
            return m_Ptr;
        }
            
        inline const T *operator->() const
        {
            return m_Ptr;
        }
            
        inline const T *ptr() const
        {
            return m_Ptr;
        }
            
        inline T *ptr()
        {
            return m_Ptr;
        }
            
        inline const T *operator*() const
        {
            return m_Ptr;
        }
            
        inline operator bool() const
        {
            return m_Ptr != nullptr;
        }
        
    private:
        ReferenceBase* m_Test = nullptr;
        T* m_Ptr;
    };
    
    template<class T>
    class WeakReference : ReferenceBase
    {
    public:
        WeakReference(T* ptr)
        {
            m_Ptr = ptr;
            reference();
        }
        
        ~WeakReference()
        {
            if(unreference())
                delete m_Ptr;
        }
        
    private:
        T* m_Ptr;
    };
    
    template<class T>
    class Owned : ReferenceBase
    {
    public:
        Owned(T* ptr)
        {
            m_Ptr = ptr;
            reference();
        }
        
        ~Owned()
        {
            if(unreference())
                delete m_Ptr;
        }
        
    private:
        T* m_Ptr;

    };
            
#ifdef CUSTOM_SMART_PTR
    
    template<class T>
    using Ref = Reference<T>;
    
    template <typename T, typename ... Args>
    Ref<T> CreateRef(Args&& ...args)
    {
        return Reference::CreateRef<T>(args ...);
    }
    
    template <typename T>
    Ref<T> CreateRef(T* t)
    {
        return Reference<T>(t);
    }
#else
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
#endif
}
