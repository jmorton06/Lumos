#pragma once
#include "Core/Core.h"
#include "Core/LMLog.h"
#include <mutex>

namespace Lumos
{
    template <class T>
    class TSingleton
    {
    public:
        //Provide global access to the only instance of this class
        static T& Get()
        {
            if(!m_pInstance) //This if statement prevents the costly Lock-step being required each time the instance is requested
            {
                m_pInstance = new T();
            }
            return *m_pInstance;
        }

        //Provide global access to release/delete this class
        static void Release()
        {
            if(m_pInstance)
            {
                delete m_pInstance;
                m_pInstance = nullptr;
            }
        }

    protected:
        //Only allow the class to be created and destroyed by itself
        TSingleton() { }
        ~TSingleton() { }

        static T* m_pInstance;

    private:
        NONCOPYABLE(TSingleton);
    };

    //Finally make sure that the instance is initialised to NULL at the start of the program
    template <class T>
    T* TSingleton<T>::m_pInstance = nullptr;

    template <class T>
    class TSingletonInit
    {
    public:
        //Provide global access to the only instance of this class
        static T& Get()
        {
            LUMOS_ASSERT(m_pInstance == nullptr, "Singleton hasn't been Initialised");
            return *m_pInstance;
        }

        template <typename... TArgs>
        static void Init(TArgs... args)
        {
            LUMOS_ASSERT(m_pInstance == nullptr, "Calling Init twice");
            m_pInstance = new T(std::forward<TArgs>(args)...);
        }

        //Provide global access to release/delete this class
        static void Release()
        {
            if(m_pInstance)
            {
                delete m_pInstance;
                m_pInstance = nullptr;
            }
        }

    protected:
        //Only allow the class to be created and destroyed by itself
        TSingletonInit() { }
        ~TSingletonInit() { }

        static T* m_pInstance;

    private:
        NONCOPYABLE(TSingletonInit);
    };

    template <class T>
    T* TSingletonInit<T>::m_pInstance = nullptr;

    template <class T>
    class ThreadSafeSingleton
    {
    public:
        //Provide global access to the only instance of this class
        static T& Get()
        {
            if(!m_pInstance) //This if statement prevents the costly Lock-step being required each time the instance is requested
            {
                std::lock_guard<std::mutex> lock(m_mConstructed); //Lock is required here though, to prevent multiple threads initialising multiple instances of the class when it turns out it has not been initialised yet
                if(!m_pInstance) //Check to see if a previous thread has already initialised an instance in the time it took to acquire a lock.
                {
                    m_pInstance = new T();
                }
            }
            return *m_pInstance;
        }

        //Provide global access to release/delete this class
        static void Release()
        {
            //Technically this could have another enclosing if statement, but speed is much less of a problem as this should only be called once in the entire program.
            std::lock_guard<std::mutex> lock(m_mConstructed);
            if(m_pInstance)
            {
                delete m_pInstance;
                m_pInstance = nullptr;
            }
        }

    protected:
        //Only allow the class to be created and destroyed by itself
        ThreadSafeSingleton() { }
        ~ThreadSafeSingleton() { }

        static T* m_pInstance;
        //Keep a static instance pointer to refer to as required by the rest of the program
        static std::mutex m_mConstructed;

    private:
        NONCOPYABLE(ThreadSafeSingleton);
    };

    //Finally make sure that the instance is initialised to NULL at the start of the program
    template <class T>
    std::mutex ThreadSafeSingleton<T>::m_mConstructed;
    template <class T>
    T* ThreadSafeSingleton<T>::m_pInstance = nullptr;
}
