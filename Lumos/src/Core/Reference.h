#pragma once

namespace Lumos
{
    template<T>
    class Reference
    {
        
    private:
        T* m_Ptr;
    };
    
    template<T>
    class WeakReference
    {
        
    private:
        T* m_Ptr;
    };
    
    template<T>
    class Owned
    {
        
    private:
        T* m_Ptr;

    };
}
