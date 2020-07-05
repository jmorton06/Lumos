#pragma once
#include "lmpch.h"
#include "Core/VFS.h"
#include "Audio/Sound.h"

namespace Lumos
{
	template<class T>
	class AssetManager
	{
	public:
		AssetManager() = default;
		~AssetManager() = default;

		void LoadAsset(const std::string& name, const std::string& filePath)
		{
			Lumos::Debug::Log::Error("Unsupported Loading Type");
		}

		void Add(const std::string& name, const Ref<T>& asset);

		Ref<T> Get(const std::string& name);

		bool Exists(const std::string& name) const;

		_ALWAYS_INLINE_ void Clear()
		{
			m_Assets.clear();
		}

	private:
		std::unordered_map<std::string, Ref<T>> m_Assets;

		NONCOPYABLE(AssetManager)

		typedef typename std::unordered_map<std::string, Ref<T>>::const_iterator const_iterator;
		const_iterator begin() const
		{
			return m_Assets.begin();
		}
		const_iterator end() const
		{
			return m_Assets.end();
		}
	};

	template<class T>
	void AssetManager<T>::Add(const std::string& name, const Ref<T>& asset)
	{
		LUMOS_ASSERT(m_Assets.find(name) == m_Assets.end(), "Adding asset with the same name");
		m_Assets[name] = asset;
	}

	template<class T>
	Ref<T> AssetManager<T>::Get(const std::string& name)
	{
		const typename std::unordered_map<std::string, Ref<T>>::iterator s = m_Assets.find(name);
		return (s != m_Assets.end() ? s->second : nullptr);
	}

	template<class T>
	bool AssetManager<T>::Exists(const std::string& name) const
	{
		const typename std::unordered_map<std::string, Ref<T>>::iterator s = m_Assets.find(name);
		return s != m_Assets.end();
	}

	template<>
	_FORCE_INLINE_ void LUMOS_EXPORT AssetManager<Sound>::LoadAsset(const std::string& name, const std::string& filePath)
	{
		std::string physicalPath;
		if(!Lumos::VFS::Get()->ResolvePhysicalPath(filePath, physicalPath))
		{
			LUMOS_LOG_CRITICAL("Could not load Audio File : ", filePath);
		}

		std::string extension = physicalPath.substr(physicalPath.length() - 3, 3);

		auto s = Sound::Create(physicalPath, extension);
		Add(name, Ref<Sound>(s));
	}

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

	private:
		inline static MapType m_nameResourceMap = {};
		static LoadFunc m_loadFunc;
		static ReleaseFunc m_releaseFunc;
		static ReloadFunc m_reloadFunc;
		static GetIdFunc m_getIdFunc;
	};
}
