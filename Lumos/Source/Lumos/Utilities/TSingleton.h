#pragma once
#include "Core/Core.h"
#include "Core/Mutex.h"

namespace Lumos
{
    template <class T>
    class TSingleton
    {
    public:
        // Provide global access to the only instance of this class
        static T& Get()
        {
            if(!m_pInstance) // This if statement prevents the costly Lock-step being required each time the instance is requested
            {
                m_pInstance = new T();
            }
            return *m_pInstance;
        }

        static void SetInstance(T* instance)
        {
            Release();
            m_pInstance = instance;
        }

        // Provide global access to release/delete this class
        static void Release()
        {
            if(m_pInstance)
            {
                delete m_pInstance;
                m_pInstance = nullptr;
            }
        }

    protected:
        // Only allow the class to be created and destroyed by itself
        TSingleton() { }
        ~TSingleton() { }

        static T* m_pInstance;

    private:
        NONCOPYABLE(TSingleton);
    };

    // Finally make sure that the instance is initialised to NULL at the start of the program
    template <class T>
    T* TSingleton<T>::m_pInstance = nullptr;

    template <class T>
    class TSingletonAbstract
    {
    public:
        // Provide global access to the only instance of this class
        static T& Get()
        {
            ASSERT(m_pInstance != nullptr, "Singleton hasn't been Created");
            return *m_pInstance;
        }

        static T* GetPtr()
        {
            ASSERT(m_pInstance != nullptr, "Singleton hasn't been Created");
            return m_pInstance;
        }

        static void SetInstance(T* instance)
        {
            Release();
            m_pInstance = instance;
        }

        // Provide global access to release/delete this class
        static void Release()
        {
            if(m_pInstance)
            {
                delete m_pInstance;
                m_pInstance = nullptr;
            }
        }

    protected:
        // Only allow the class to be created and destroyed by itself
        TSingletonAbstract() { }
        ~TSingletonAbstract() { }

        static T* m_pInstance;

    private:
        NONCOPYABLE(TSingletonAbstract);
    };

    // Finally make sure that the instance is initialised to NULL at the start of the program
    template <class T>
    T* TSingletonAbstract<T>::m_pInstance = nullptr;

    template <class T>
    class TSingletonInit
    {
    public:
        // Provide global access to the only instance of this class
        static T& Get()
        {
            ASSERT(m_pInstance == nullptr, "Singleton hasn't been Initialised");
            return *m_pInstance;
        }

        template <typename... TArgs>
        static void Init(TArgs... args)
        {
            ASSERT(m_pInstance == nullptr, "Calling Init twice");
            m_pInstance = new T(Forward<TArgs>(args)...);
        }

        // Provide global access to release/delete this class
        static void Release()
        {
            if(m_pInstance)
            {
                delete m_pInstance;
                m_pInstance = nullptr;
            }
        }

        static void SetInstance(T* instance)
        {
            Release();
            m_pInstance = instance;
        }

    protected:
        // Only allow the class to be created and destroyed by itself
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
        // Provide global access to the only instance of this class
        static T& Get()
        {
            if(!m_pInstance) // This if statement prevents the costly Lock-step being required each time the instance is requested
            {
                m_mConstructed = new Mutex();
                MutexInit(m_mConstructed);
                ScopedMutex mutex(m_mConstructed); // Lock is required here though, to prevent multiple threads initialising multiple instances of the class when it turns out it has not been initialised yet
                if(!m_pInstance)                   // Check to see if a previous thread has already initialised an instance in the time it took to acquire a lock.
                {
                    m_pInstance = new T();
                }
            }
            return *m_pInstance;
        }

        // Provide global access to release/delete this class
        static void Release()
        {
            // Technically this could have another enclosing if statement, but speed is much less of a problem as this should only be called once in the entire program.
            if(m_pInstance)
            {
                MutexLock(m_mConstructed);

                delete m_pInstance;
                m_pInstance = nullptr;

                MutexUnlock(m_mConstructed);
                MutexDestroy(m_mConstructed);

                delete m_mConstructed;
                m_mConstructed = nullptr;
            }
        }

        static void SetInstance(T* instance)
        {
            Release();
            m_pInstance = instance;
        }

    protected:
        // Only allow the class to be created and destroyed by itself
        ThreadSafeSingleton() { }
        ~ThreadSafeSingleton() { }

        static T* m_pInstance;
        // Keep a static instance pointer to refer to as required by the rest of the program
        static Mutex* m_mConstructed;

    private:
        NONCOPYABLE(ThreadSafeSingleton);
    };

    // Finally make sure that the instance is initialised to NULL at the start of the program
    template <class T>
    Mutex* ThreadSafeSingleton<T>::m_mConstructed;
    template <class T>
    T* ThreadSafeSingleton<T>::m_pInstance = nullptr;
}
