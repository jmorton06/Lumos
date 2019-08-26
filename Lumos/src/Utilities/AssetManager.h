#pragma once
#include "LM.h"

namespace Lumos
{
	template <class T>
	class AssetManager
	{
	public:
		AssetManager() {};
		~AssetManager() {};

		void AddAsset(const String& name, const Ref<T>& asset);
		Ref<T> GetAsset(const String& name);

		void Clear()
		{
			m_Assets.clear();
		}

	private:
		std::map<String, Ref<T>> m_Assets;

        AssetManager(AssetManager const&) = delete;
        AssetManager& operator=(AssetManager const&) = delete;

		typedef typename std::map<String, Ref<T>>::const_iterator const_iterator;
		const_iterator begin() const { return m_Assets.begin(); }
		const_iterator end()   const { return m_Assets.end(); }
	};

	template <class T>
	void AssetManager<T>::AddAsset(const String& name, const Ref<T>& asset)
	{
		m_Assets.insert(make_pair(name, asset));
	}

	template <class T>
	Ref<T> AssetManager<T>::GetAsset(const String& name)
	{
		const typename std::map<String, Ref<T>>::iterator s = m_Assets.find(name);
		return (s != m_Assets.end() ? s->second : nullptr);
	}
}
