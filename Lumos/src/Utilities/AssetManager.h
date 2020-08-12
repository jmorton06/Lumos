#pragma once
#include "lmpch.h"
#include "Core/VFS.h"
#include "Audio/Sound.h"

namespace Lumos
{
	template<typename T>
	class ResourceManager
	{
	public:
		typedef T Type;
		typedef std::string IDType;
		typedef Ref<T> ResourceHandle;

		struct Resource
		{
			float timeSinceReload;
			ResourceHandle data;
			bool onDisk;
		};

		typedef std::unordered_map<IDType, Resource> MapType;

		typedef std::function<bool(const IDType&, T&)> LoadFunc;
		typedef std::function<void(T&)> ReleaseFunc;
		typedef std::function<bool(const IDType&, T&)> ReloadFunc;
		typedef std::function<IDType(const T&)> GetIdFunc;

		static ResourceHandle GetResource(const IDType& name)
		{
			typename MapType::iterator itr = m_nameResourceMap.find(name);
			if(itr != m_nameResourceMap.end())
			{
				return itr->second.data;
			}

			ResourceHandle resourceData(new T);
			if(!m_loadFunc(name, *resourceData))
			{
				std::cerr << "ERROR: Resource Manager could not load resource name \"" << name << "\" of type " << typeid(T).name() << std::endl;
				return ResourceHandle(nullptr);
			}

			Resource newResource;
			newResource.data = resourceData;
			newResource.timeSinceReload = 0;
			newResource.onDisk = true;
			m_nameResourceMap.emplace(name, newResource);

			return resourceData;
		}

		static ResourceHandle GetResource(const T& data)
		{
			IDType newId = m_getIdFunc(data);

			typename MapType::iterator itr = m_nameResourceMap.find(newId);
			if(itr == m_nameResourceMap.end())
			{
				ResourceHandle resourceData(new T);
				*resourceData = data;

				Resource newResource;
				newResource.data = resourceData;
				newResource.timeSinceReload = 0;
				newResource.onDisk = false;
				m_nameResourceMap.emplace(newId, newResource);

				return resourceData;
			}

			return itr->second.data;
		}

		static void Destroy()
		{
			typename MapType::iterator itr = m_nameResourceMap.begin();
			while(itr != m_nameResourceMap.end())
			{
				m_releaseFunc(*(itr->second.data));
				++itr;
			}
		}

		static void Update(const float elapsedMilliseconds)
		{
			typename MapType::iterator itr = m_nameResourceMap.begin();

			while(itr != m_nameResourceMap.end())
			{
				if(itr->second.data.use_count() == 1)
					itr = m_nameResourceMap.erase(itr);
				else
					++itr;
			}
		}

		static bool ReloadResources()
		{
			typename MapType::iterator itr = m_nameResourceMap.begin();
			while(itr != m_nameResourceMap.end())
			{
				itr->second.timeSinceReload = 0;
				if(!m_reloadFunc(itr->first, *(itr->second.data)))
				{
					std::cerr << "ERROR: Resource Manager could not RE-load resource \"" << itr->first << "\" of type " << typeid(T).name() << std::endl;
					// some can't be loaded because they weren't files but data
				}
				++itr;
			}
			return true;
		}

		static LoadFunc& LoadFunction() { return m_loadFunc; }
		static ReleaseFunc& ReleaseFunction() { return m_releaseFunc; }
		static ReloadFunc& ReloadFunction() { return m_reloadFunc; }


	private:
		inline static MapType m_nameResourceMap = {};
		static LoadFunc m_loadFunc;
		static ReleaseFunc m_releaseFunc;
		static ReloadFunc m_reloadFunc;
		static GetIdFunc m_getIdFunc;
	};
}
