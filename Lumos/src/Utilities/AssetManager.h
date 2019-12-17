#pragma once
#include "lmpch.h"
#include "Core/VFS.h"

namespace Lumos
{
	template <class T>
	class AssetManager
	{
	public:
		AssetManager() = default;
		~AssetManager() = default;

		void Add(const String& name, const Ref<T>& asset);
        
		Ref<T> Get(const String& name);
        
        bool Exists(const String& name) const;

		_ALWAYS_INLINE_ void Clear()
		{
			m_Assets.clear();
		}

	private:
		std::unordered_map<String, Ref<T>> m_Assets;

        NONCOPYABLE(AssetManager)

		typedef typename std::unordered_map<String, Ref<T>>::const_iterator const_iterator;
		const_iterator begin() const { return m_Assets.begin(); }
		const_iterator end()   const { return m_Assets.end(); }
	};

	template <class T>
	void AssetManager<T>::Add(const String& name, const Ref<T>& asset)
	{
        LUMOS_ASSERT(m_Assets.find(name) == m_Assets.end(), "Adding asset with the same name");
		m_Assets[name] = asset;
	}

	template <class T>
	Ref<T> AssetManager<T>::Get(const String& name)
	{
		const typename std::unordered_map<String, Ref<T>>::iterator s = m_Assets.find(name);
		return (s != m_Assets.end() ? s->second : nullptr);
	}
    
    template <class T>
    bool AssetManager<T>::Exists(const String& name) const
    {
        const typename std::unordered_map<String, Ref<T>>::iterator s = m_Assets.find(name);
        return s != m_Assets.end();
    }
}
