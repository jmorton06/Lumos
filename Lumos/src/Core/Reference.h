#pragma once

#include "LM.h"
#include "LMLog.h"
#include "ReferenceCounter.h"
#include <memory>

namespace Lumos
{
    class LUMOS_EXPORT RefCount
    {
    public:
        RefCount();
        ~RefCount();
        
        inline bool IsReferenced() const { return m_RefcountInit.get() < 1; }
        bool InitRef();
        bool reference(); // returns false if refcount is at zero and didn't get increased
        bool unreference();
        int GetReferenceCount() const;
    private:
        ReferenceCounter m_Refcount;
        ReferenceCounter m_RefcountInit;
    };
    
    template<class T>
    class LUMOS_EXPORT Reference
    {
    public:
        Reference(std::nullptr_t)
        {
            m_Ptr = nullptr;
            m_Counter = nullptr;
        }
        
        Reference(T* ptr = nullptr)
        {
            if(ptr)
                refPointer(ptr);
        }
        
        Reference(const Reference& other)
        {
            ref(other);
        }
        
        ~Reference()
        {
            if(m_Counter != nullptr)
            {
                if(m_Counter->unreference())
                {
                    delete m_Counter;
                    if(m_Ptr)
                        delete m_Ptr;
                }
            }
        }
        
		// Access to smart pointer state
		_FORCE_INLINE_ T* get()                 const { return m_Ptr; }
		_FORCE_INLINE_ explicit operator bool() const { return m_Ptr; }
        _FORCE_INLINE_ RefCount* GetCounter()   const { return m_Counter; }

        _FORCE_INLINE_ T* release() noexcept
        {
            T* tmp = nullptr;
            
            if(m_Counter->unreference())
            {
                delete m_Counter;
                m_Counter = nullptr;
            }
            
            std::swap(tmp, m_Ptr);
            m_Ptr = nullptr;
            
            return tmp;
        }
        
        _FORCE_INLINE_ void reset(T *p_ptr = nullptr)
        {
            if(m_Counter != nullptr)
            {
                if(m_Counter->unreference())
                {
                    delete m_Ptr;
                    delete m_Counter;
                }
            }
            
            m_Ptr = p_ptr;
            m_Counter = new RefCount();
            m_Counter->InitRef();
        }

		_FORCE_INLINE_ void operator=(Reference const& rhs)
		{
			ref(rhs);
		}
        
        _FORCE_INLINE_ Reference& operator=(T* newData)
        {
            if(newData != nullptr)
                refPointer(newData);
            return *this;
        }
        
        template<typename U>
        _FORCE_INLINE_ Reference(const Reference<U>& moving)
        {
            U* movingPtr = moving.get();
            
            T* castPointer = static_cast<T*>(movingPtr);
            
            if(castPointer != nullptr)
            {
                if (moving.get() == m_Ptr)
                    return;
                
                if(m_Counter != nullptr)
                {
                    if (m_Counter->unreference())
                    {
                        delete m_Ptr;
                        delete m_Counter;
                    }
                }
                
                if(moving.GetCounter() && moving.get())
                {
                    m_Ptr = moving.get();
                    m_Counter = moving.GetCounter();
                    m_Counter->reference();
                }
            }
            else
            {
                LUMOS_CORE_ERROR("Failed to cast Reference");
            }
        }
        
        template<typename U>
        _FORCE_INLINE_ Reference& operator=(const Reference<U>& moving)
        {
            U* movingPtr = moving.get();
            
            T* castPointer = dynamic_cast<T*>(movingPtr);
            
            if(castPointer != nullptr)
            {
                if(m_Counter != nullptr)
                {
                    if (m_Counter->unreference())
                    {
                        delete m_Ptr;
                        delete m_Counter;
                    }
                }
                
                if(moving.GetCounter() && moving.get())
                {
                    m_Ptr = moving.get();
                    m_Counter = moving.GetCounter();
                    m_Counter->reference();
                }
            }
            else
            {
                LUMOS_CORE_ERROR("Failed to cast Reference");
            }
            
            return *this;
        }
        
        _FORCE_INLINE_ bool operator==(const T *p_ptr)			const { return m_Ptr == p_ptr; }
        _FORCE_INLINE_ bool operator!=(const T *p_ptr)			const { return m_Ptr != p_ptr; }
        _FORCE_INLINE_ bool operator<(const Reference<T> &p_r)	const { return m_Ptr < p_r.m_Ptr; }
        _FORCE_INLINE_ bool operator==(const Reference<T> &p_r)	const { return m_Ptr == p_r.m_Ptr; }
        _FORCE_INLINE_ bool operator!=(const Reference<T> &p_r)	const { return m_Ptr != p_r.m_Ptr; }

        _FORCE_INLINE_ void swap(Reference& other) noexcept
        {
            std::swap(m_Ptr,  other.m_Ptr);
            std::swap(m_Counter, other.m_Counter);
        }
        // Const correct access owned object
		_FORCE_INLINE_ T* operator->() const { return &*m_Ptr; }
		_FORCE_INLINE_ T& operator*()  const { return *m_Ptr; }

    private:
            
        _FORCE_INLINE_ void ref(const Reference &p_from)
        {
            if (p_from.m_Ptr == m_Ptr)
                return;
            
            if(m_Counter != nullptr)
            {
                if (m_Counter->unreference())
                {
                    delete m_Ptr;
                    delete m_Counter;
                }
            }
            
            if(p_from.GetCounter() && p_from.get())
            {
                m_Ptr = p_from.get();
                m_Counter = p_from.GetCounter();
                m_Counter->reference();
            }
        }
            
        _FORCE_INLINE_ void refPointer(T* ptr)
        {
            LUMOS_CORE_ASSERT(ptr, "Creating shared ptr with nullptr");
            
            m_Ptr = ptr;
            m_Counter = new RefCount();
            m_Counter->InitRef();
        }
            
        RefCount* m_Counter = nullptr;
        T* m_Ptr;
    };
    
    template<class T>
    class LUMOS_EXPORT WeakReference
    {
    public:
        WeakReference(T* ptr)
        {
            m_Ptr = ptr;
        }
        
        ~WeakReference()
        {
        }
        
    private:
        T* m_Ptr;
    };
    
    template<class T>
    class LUMOS_EXPORT Owned
    {
    public:
        Owned(T* ptr)
        {
            m_Ptr = ptr;
        }
        
        ~Owned()
        {
            delete m_Ptr;
        }
        
        Owned(Owned const&)            = delete;
        Owned& operator=(Owned const&) = delete;
        
        // Const correct access owned object
        T* operator->() const {return m_Ptr;}
        T& operator*()  const {return *m_Ptr;}
        
        // Access to smart pointer state
        T* get()                 const {return m_Ptr;}
        explicit operator bool() const {return m_Ptr;}
        
        // Modify object state
        T* release()
        {
            T* result = nullptr;
            std::swap(result, m_Ptr);
            return result;
        }
        
    private:
        T* m_Ptr;

    };
           
#define CUSTOM_SMART_PTR
#ifdef CUSTOM_SMART_PTR
    
    template<class T>
    using Ref = Reference<T>;
    
    template <typename T, typename ... Args>
    Ref<T> CreateRef(Args&& ...args)
    {
        auto ptr = new T(args ...);
        
        return Reference<T>(ptr);
    }
            
    template<class T>
    using Scope = std::unique_ptr<T>;
    
    template <typename T, typename ... Args>
    Scope<T> CreateScope(Args&& ...args)
    {
        return std::make_unique<T>(args ...);
    }
    
    template <typename T>
    Scope<T> CreateScope(T* t)
    {
        return std::unique_ptr<T>(t);
    }
#else
    template<class T>
    using Ref = std::shared_ptr<T>;
    
    template <typename T, typename ... Args>
    Ref<T> CreateRef(Args&& ...args)
    {
        return std::make_shared<T>(args ...);
    }
            
    template<class T>
    using WeakRef = std::weak_ptr<T>;

    template<class T>
    using Scope = std::unique_ptr<T>;
    
    template <typename T, typename ... Args>
    Scope<T> CreateScope(Args&& ...args)
    {
        return std::make_unique<T>(args ...);
    }
    
    template <typename T>
    Scope<T> CreateScope(T* t)
    {
        return std::unique_ptr<T>(t);
    }
#endif
}
