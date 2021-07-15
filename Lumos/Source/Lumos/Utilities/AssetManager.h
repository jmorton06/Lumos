#pragma once

#include "Core/VFS.h"
#include "Core/Engine.h"
#include "Audio/Sound.h"
#include "Graphics/RHI/Shader.h"
#include "Utilities/TSingleton.h"

namespace Lumos
{
    template <typename T>
    class ResourceManager
    {
    public:
        typedef T Type;
        typedef std::string IDType;
        typedef SharedRef<T> ResourceHandle;

        struct Resource
        {
            float timeSinceReload;
            float lastAccessed;
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
            typename MapType::iterator itr = m_NameResourceMap.find(name);
            if(itr != m_NameResourceMap.end())
            {
                itr->second.lastAccessed = Engine::GetTimeStep().GetElapsedSeconds();
                return itr->second.data;
            }

            ResourceHandle resourceData;
            if(!m_LoadFunc(name, resourceData))
            {
                LUMOS_LOG_ERROR("Resource Manager could not load resource name {0} of type {1}", name, typeid(T).name());
                return ResourceHandle(nullptr);
            }

            Resource newResource;
            newResource.data = resourceData;
            newResource.timeSinceReload = 0;
            newResource.onDisk = true;
            newResource.lastAccessed = Engine::GetTimeStep().GetElapsedSeconds();

            m_NameResourceMap.emplace(name, newResource);

            return resourceData;
        }

        ResourceHandle GetResource(const ResourceHandle& data)
        {
            IDType newId = m_GetIdFunc(data);

            typename MapType::iterator itr = m_NameResourceMap.find(newId);
            if(itr == m_NameResourceMap.end())
            {
                ResourceHandle resourceData = data;

                Resource newResource;
                newResource.data = resourceData;
                newResource.timeSinceReload = 0;
                newResource.onDisk = false;
                m_NameResourceMap.emplace(newId, newResource);

                return resourceData;
            }

            return itr->second.data;
        }

        void Destroy()
        {
            typename MapType::iterator itr = m_NameResourceMap.begin();
            while(itr != m_NameResourceMap.end())
            {
                m_ReleaseFunc((itr->second.data));
                ++itr;
            }
        }

        void Update(const float elapsedMilliseconds)
        {
            typename MapType::iterator itr = m_NameResourceMap.begin();

            float now = Engine::GetTimeStep().GetElapsedSeconds();
            while(itr != m_NameResourceMap.end())
            {
                if(itr->second.data.GetCounter()->GetReferenceCount() == 1 && m_ExpirationTime < (now - itr->second.lastAccessed))
                    itr = m_NameResourceMap.erase(itr);
                else
                    ++itr;
            }
        }

        bool ReloadResources()
        {
            typename MapType::iterator itr = m_NameResourceMap.begin();
            while(itr != m_NameResourceMap.end())
            {
                itr->second.timeSinceReload = 0;
                if(!m_ReloadFunc(itr->first, (itr->second.data)))
                {
                    LUMOS_LOG_ERROR("Resource Manager could not reload resource name {0} of type {1}", itr->first, typeid(T).name());
                }
                ++itr;
            }
            return true;
        }

        bool ResourceExists(const IDType& name)
        {
            typename MapType::iterator itr = m_NameResourceMap.find(name);
            return itr != m_NameResourceMap.end();
        }

        ResourceHandle operator[](const IDType& name)
        {
            return GetResource(name);
        }

        LoadFunc& LoadFunction() { return m_LoadFunc; }
        ReleaseFunc& ReleaseFunction() { return m_ReleaseFunc; }
        ReloadFunc& ReloadFunction() { return m_ReloadFunc; }

    protected:
        MapType m_NameResourceMap = {};
        LoadFunc m_LoadFunc;
        ReleaseFunc m_ReleaseFunc;
        ReloadFunc m_ReloadFunc;
        GetIdFunc m_GetIdFunc;
        float m_ExpirationTime = 3.0f;
    };

    class ShaderLibrary : public ResourceManager<Graphics::Shader>
    {
    public:
        ShaderLibrary()
        {
            m_LoadFunc = Load;
        }

        ~ShaderLibrary()
        {
        }

        static bool Load(const std::string& filePath, SharedRef<Graphics::Shader>& shader)
        {
            shader = SharedRef<Graphics::Shader>(Graphics::Shader::CreateFromFile(filePath));
            return true;
        }
    };
}
