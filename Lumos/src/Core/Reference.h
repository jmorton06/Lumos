#pragma once

#include <memory>

namespace Lumos
{
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
