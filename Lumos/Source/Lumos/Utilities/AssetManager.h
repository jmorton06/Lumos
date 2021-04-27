#pragma once

#include "Core/VFS.h"
#include "Audio/Sound.h"
#include "Graphics/API/Shader.h"
#include "Utilities/TSingleton.h"

namespace Lumos
{
    template <typename T>
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

        typedef std::function<bool(const IDType&, ResourceHandle&)> LoadFunc;
        typedef std::function<void(ResourceHandle&)> ReleaseFunc;
        typedef std::function<bool(const IDType&, ResourceHandle&)> ReloadFunc;
        typedef std::function<IDType(const ResourceHandle&)> GetIdFunc;

        ResourceHandle GetResource(const IDType& name)
        {
            typename MapType::iterator itr = m_nameResourceMap.find(name);
            if(itr != m_nameResourceMap.end())
            {
                return itr->second.data;
            }

            ResourceHandle resourceData;
            if(!m_loadFunc(name, resourceData))
            {
                LUMOS_LOG_ERROR("Resource Manager could not load resource name {0} of type {1}", name, typeid(T).name());
                return ResourceHandle(nullptr);
            }

            Resource newResource;
            newResource.data = resourceData;
            newResource.timeSinceReload = 0;
            newResource.onDisk = true;
            m_nameResourceMap.emplace(name, newResource);

            return resourceData;
        }

        ResourceHandle GetResource(const ResourceHandle& data)
        {
            IDType newId = m_getIdFunc(data);

            typename MapType::iterator itr = m_nameResourceMap.find(newId);
            if(itr == m_nameResourceMap.end())
            {
                ResourceHandle resourceData = data;

                Resource newResource;
                newResource.data = resourceData;
                newResource.timeSinceReload = 0;
                newResource.onDisk = false;
                m_nameResourceMap.emplace(newId, newResource);

                return resourceData;
            }

            return itr->second.data;
        }

        void Destroy()
        {
            typename MapType::iterator itr = m_nameResourceMap.begin();
            while(itr != m_nameResourceMap.end())
            {
                m_releaseFunc((itr->second.data));
                ++itr;
            }
        }

        void Update(const float elapsedMilliseconds)
        {
            typename MapType::iterator itr = m_nameResourceMap.begin();

            while(itr != m_nameResourceMap.end())
            {
                if(itr->second.data.GetCounter()->GetReferenceCount() == 1)
                    itr = m_nameResourceMap.erase(itr);
                else
                    ++itr;
            }
        }

        bool ReloadResources()
        {
            typename MapType::iterator itr = m_nameResourceMap.begin();
            while(itr != m_nameResourceMap.end())
            {
                itr->second.timeSinceReload = 0;
                if(!m_reloadFunc(itr->first, (itr->second.data)))
                {
                    LUMOS_LOG_ERROR("Resource Manager could not reload resource name {0} of type {1}", itr->first, typeid(T).name());
                }
                ++itr;
            }
            return true;
        }

        bool ResourceExists(const IDType& name)
        {
            typename MapType::iterator itr = m_nameResourceMap.find(name);
            return itr != m_nameResourceMap.end();
        }

        ResourceHandle operator[](const IDType& name)
        {
            return GetResource(name);
        }

        LoadFunc& LoadFunction() { return m_loadFunc; }
        ReleaseFunc& ReleaseFunction() { return m_releaseFunc; }
        ReloadFunc& ReloadFunction() { return m_reloadFunc; }

    protected:
        MapType m_nameResourceMap = {};
        LoadFunc m_loadFunc;
        ReleaseFunc m_releaseFunc;
        ReloadFunc m_reloadFunc;
        GetIdFunc m_getIdFunc;
    };

    class ShaderLibrary : public ResourceManager<Graphics::Shader>
    {
    public:
        ShaderLibrary()
        {
            m_loadFunc = Load;
        }

        ~ShaderLibrary()
        {
        }

        static bool Load(const std::string& filePath, Ref<Graphics::Shader>& shader)
        {
            shader = Ref<Graphics::Shader>(Graphics::Shader::CreateFromFile(filePath));
            return true;
        }
    };
}
