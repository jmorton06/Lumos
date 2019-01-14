#pragma once
#include "LM.h"
#include <mutex>

namespace Lumos
{

	template <class T>
	class TSingleton
	{
	public:
		//Provide global access to the only instance of this class
		static T* Instance()
		{
			if (!m_pInstance)	//This if statement prevents the costly Lock-step being required each time the instance is requested
			{
				std::lock_guard<std::mutex> lock(m_mConstructed);		//Lock is required here though, to prevent multiple threads initialising multiple instances of the class when it turns out it has not been initialised yet
				if (!m_pInstance) //Check to see if a previous thread has already initialised an instance in the time it took to acquire a lock.
				{
					m_pInstance = new T();
				}
			}
			return m_pInstance;
		}

		//Provide global access to release/delete this class
		static void Release()
		{
			//Technically this could have another enclosing if statement, but speed is much less of a problem as this should only be called once in the entire program.
			std::lock_guard<std::mutex> lock(m_mConstructed);
			if (m_pInstance)
			{
				delete m_pInstance;
				m_pInstance = nullptr;
			}
		}

	protected:
		//Only allow the class to be created and destroyed by itself
		TSingleton() {}
		~TSingleton() {}

		static T* m_pInstance;
		//Keep a static instance pointer to refer to as required by the rest of the program
		static std::mutex m_mConstructed;

	private:
		//Prevent the class from being copied either by '=' operator or by copy constructor
		TSingleton(TSingleton const&) = delete;
		TSingleton& operator=(TSingleton const&) = delete;
	};

	//Finally make sure that the instance is initialised to NULL at the start of the program
	template <class T> std::mutex TSingleton<T>::m_mConstructed;
	template <class T> T* TSingleton<T>::m_pInstance = nullptr;
}