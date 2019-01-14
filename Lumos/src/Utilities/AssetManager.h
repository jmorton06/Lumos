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

		void AddAsset(const String& name, std::shared_ptr<T> asset);
		std::shared_ptr<T> GetAsset(const String& name);

		void Clear()
		{
			m_Assets.clear();
		}

	private:
		std::map<String, std::shared_ptr<T>> m_Assets;

		AssetManager(AssetManager const&) {}
		AssetManager& operator=(AssetManager const&) { return {}; }

		typedef typename std::map<String, std::shared_ptr<T>>::const_iterator const_iterator;
		const_iterator begin() const { return m_Assets.begin(); }
		const_iterator end()   const { return m_Assets.end(); }
	};

	template <class T>
	void AssetManager<T>::AddAsset(const String& name, std::shared_ptr<T> asset)
	{
		m_Assets.insert(make_pair(name, asset));
	}

	template <class T>
	std::shared_ptr<T> AssetManager<T>::GetAsset(const String& name)
	{
		const typename std::map<String, std::shared_ptr<T>>::iterator s = m_Assets.find(name);
		return (s != m_Assets.end() ? s->second : nullptr);
	}
}