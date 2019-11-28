#pragma once

#include "lmpch.h"
#include "ReferenceCounter.h"
#include "OS/Memory.h"
#include "Core/LMLog.h"

namespace Lumos
{
    class LUMOS_EXPORT RefCount
    {
    public:
        RefCount();
        ~RefCount();
        
        _FORCE_INLINE_ bool IsReferenced() const { return m_RefcountInit.get() < 1; }
        bool InitRef();

		//Returns false if refcount is at zero and didn't get increased
        bool reference(); 
        bool unreference();

		bool weakReference();
		bool weakUnreference();

        int GetReferenceCount() const;
		int GetWeakReferenceCount() const;
    private:
        ReferenceCounter m_Refcount;
        ReferenceCounter m_RefcountInit;
		ReferenceCounter m_WeakRefcount;
    };
    
    template<class T>
    class Reference
    {
    public:
        Reference(std::nullptr_t)
        {
            m_Ptr = nullptr;
            m_Counter = nullptr;
        }
        
        Reference(T* ptr = nullptr)
        {
			m_Ptr = nullptr;
			m_Counter = nullptr;

            if(ptr)
                refPointer(ptr);
        }
        
        Reference(const Reference& other)
        {
            m_Ptr = nullptr;
            m_Counter = nullptr;
            
            ref(other);
        }
        
        Reference(Reference&& rhs) noexcept
        {
            m_Ptr = nullptr;
            m_Counter = nullptr;
            
            ref(rhs);
        }
               
        
        template<typename U>
        _FORCE_INLINE_ Reference(const Reference<U>& moving)
        {
            U* movingPtr = moving.get();
            
            T* castPointer = static_cast<T*>(movingPtr);
            
            unref();
            
            if(castPointer != nullptr)
            {
                if (moving.get() == m_Ptr)
                    return;
                
                if(moving.GetCounter() && moving.get())
                {
                    m_Ptr = moving.get();
                    m_Counter = moving.GetCounter();
                    m_Counter->reference();
                }
            }
            else
            {
				Debug::Log::Error("Failed to cast Reference");
            }
        }
        
        ~Reference()
        {
			unref();
        }
        
		// Access to smart pointer state
		_FORCE_INLINE_ T* get()                 const { return m_Ptr; }
		_FORCE_INLINE_ explicit operator bool() const { return m_Ptr != nullptr; }
        _FORCE_INLINE_ RefCount* GetCounter()   const { return m_Counter; }

        _FORCE_INLINE_ T* release() noexcept
        {
            T* tmp = nullptr;
            
            if(m_Counter->unreference())
            {
                lmdel m_Counter;
                m_Counter = nullptr;
            }
            
            std::swap(tmp, m_Ptr);
            m_Ptr = nullptr;
            
            return tmp;
        }
        
        _FORCE_INLINE_ void reset(T *p_ptr = nullptr)
        {
			unref();
            
            m_Ptr = p_ptr;
			m_Counter = nullptr;
            
            if(m_Ptr != nullptr)
            {
                m_Counter = lmnew RefCount();
                m_Counter->InitRef();
            }
        }

		_FORCE_INLINE_ void operator=(Reference const& rhs)
		{
			ref(rhs);
		}
        
        _FORCE_INLINE_ Reference& operator=(Reference&& rhs) noexcept
        {
            ref(rhs);
            return *this;
        }
        
        _FORCE_INLINE_ Reference& operator=(T* newData)
        {
            if(newData != nullptr)
                refPointer(newData);
            return *this;
        }
        
        template<typename U>
        _FORCE_INLINE_ Reference& operator=(const Reference<U>& moving)
        {
            U* movingPtr = moving.get();
            
            T* castPointer = dynamic_cast<T*>(movingPtr);
            
			unref();

            if(castPointer != nullptr)
            {
                if(moving.GetCounter() && moving.get())
                {
                    m_Ptr = moving.get();
                    m_Counter = moving.GetCounter();
                    m_Counter->reference();
                }
            }
            else
            {
                Debug::Log::Error("Failed to cast Reference");
            }
            
            return *this;
        }
        
        _FORCE_INLINE_ Reference& operator=(std::nullptr_t)
        {
            reset();
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
            
			unref();
            
            m_Counter = nullptr;
            m_Ptr = nullptr;
            
            if(p_from.GetCounter() && p_from.get())
            {
                m_Ptr = p_from.get();
                m_Counter = p_from.GetCounter();
                m_Counter->reference();
            }
        }
            
        _FORCE_INLINE_ void refPointer(T* ptr)
        {
            LUMOS_ASSERT(ptr, "Creating shared ptr with nullptr");
            
            m_Ptr = ptr;
            m_Counter = lmnew RefCount();
            m_Counter->InitRef();
        }

		_FORCE_INLINE_ void unref()
		{
			if (m_Counter != nullptr)
			{
				if (m_Counter->unreference())
				{
					lmdel m_Ptr;
					
					if(m_Counter->GetWeakReferenceCount() == 0)
						lmdel m_Counter;

					m_Ptr = nullptr;
					m_Counter = nullptr;
				}
			}
		}
            
        RefCount* m_Counter = nullptr;
        T* m_Ptr = nullptr;
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
        Owned(std::nullptr_t)
        {
            m_Ptr = nullptr;
        }
        
        Owned(T* ptr = nullptr)
        {
            m_Ptr = ptr;
        }
        
        template<typename U>
        Owned(U* ptr)
        {
            m_Ptr = dynamic_cast<T*>(ptr);
        }
        
        ~Owned()
        {
            lmdel m_Ptr;
        }
        
        Owned(Owned const&)            = delete;
        Owned& operator=(Owned const&) = delete;
        
        _FORCE_INLINE_ Owned(Owned&& moving) noexcept
        {
            moving.swap(*this);
        }
        
        _FORCE_INLINE_ Owned& operator=(Owned&& moving) noexcept
        {
            moving.swap(*this);
            return *this;
        }
        
        template<typename U>
        _FORCE_INLINE_ Owned(Owned<U>&& moving)
        {
            Owned<T> tmp(moving.release());
            tmp.swap(*this);
        }
        template<typename U>
        _FORCE_INLINE_ Owned& operator=(Owned<U>&& moving)
        {
            Owned<T> tmp(moving.release());
            tmp.swap(*this);
            return *this;
        }
        
        _FORCE_INLINE_ Owned& operator=(std::nullptr_t)
        {
            reset();
            return *this;
        }
        
        // Const correct access owned object
        T* operator->() const { return m_Ptr; }
        T& operator*()  const { return *m_Ptr; }
        
        // Access to smart pointer state
        T* get()                 const { return m_Ptr; }
        explicit operator bool() const { return m_Ptr; }
        
        // Modify object state
        _FORCE_INLINE_ T* release()
        {
            T* result = nullptr;
            std::swap(result, m_Ptr);
            return result;
        }
        
        _FORCE_INLINE_ void reset()
        {
            T* tmp = release();
            lmdel tmp;
        }
        
        _FORCE_INLINE_ void swap(Owned& src) noexcept
        {
            std::swap(m_Ptr, src.m_Ptr);
        }
        
    private:
        T* m_Ptr = nullptr;
    };
            
    template<typename T>
    void swap(Owned<T>& lhs, Owned<T>& rhs)
    {
        lhs.swap(rhs);
    }
           
#define CUSTOM_SMART_PTR
#ifdef CUSTOM_SMART_PTR
    
    template<class T>
    using Ref = Reference<T>;
    
    template <typename T, typename ... Args>
    Ref<T> CreateRef(Args&& ...args)
    {
        auto ptr = lmnew T(std::forward<Args>(args) ...);
        
        return Reference<T>(ptr);
    }
            
    template<class T>
    using Scope = Owned<T>;

    template <typename T, typename ... Args>
    Scope<T> CreateScope(Args&& ...args)
    {
        auto ptr = lmnew T(std::forward<Args>(args) ...);
        return Owned<T>(ptr);
    }

	template<class T>
	using WeakRef = WeakReference<T>;
#else
    template<class T>
    using Ref = std::shared_ptr<T>;
    
    template <typename T, typename ... Args>
    Ref<T> CreateRef(Args&& ...args)
    {
        return std::make_shared<T>(std::forward<Args>(args) ...);
    }
            
    template<class T>
    using WeakRef = std::weak_ptr<T>;

    template<class T>
    using Scope = std::unique_ptr<T>;
    
    template <typename T, typename ... Args>
    Scope<T> CreateScope(Args&& ...args)
    {
        return std::make_unique<T>(std::forward<Args>(args) ...);
    }
#endif
}
#ifdef CUSTOM_SMART_PTR
namespace std
{
    template<typename T>
    struct hash<Lumos::Reference<T>>
    {
        size_t operator()(const Lumos::Reference<T>& x) const
        {
            return hash<T*>()(x.get());
        }
    };
}
#endif
