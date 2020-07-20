#pragma once
#include "lmpch.h"
#include "Graphics/Material.h"

namespace Lumos
{
	class LUMOS_EXPORT MaterialComponent
	{
	public:
		MaterialComponent();
		explicit MaterialComponent(Ref<Material>& material);
		virtual ~MaterialComponent();

		void SetAlbedoTexture(const std::string& path);
		void SetNormalTexture(const std::string& path);
		void SetRoughnessTexture(const std::string& path);
		void SetMetallicTexture(const std::string& path);
		void SetAOTexture(const std::string& path);
		void SetEmissiveTexture(const std::string& path);

		const Ref<Material>& GetMaterial() const
		{
			return m_Material;
		}
		bool& GetActive()
		{
			return m_Active;
		}
		const bool& GetActive() const
		{
			return m_Active;
		}

		bool& GetTexturesUpdated()
		{
			return m_TexturesUpdated;
		}
		const bool& GetTexturesUpdated() const
		{
			return m_TexturesUpdated;
		}
		void SetTexturesUpdated(bool updated)
		{
			m_TexturesUpdated = updated;
		}

		template<typename Archive>
		void save(Archive& archive) const
		{
			archive(cereal::make_nvp("Material", *m_Material.get()));
		}

		template<typename Archive>
		void load(Archive& archive)
		{
			m_Material = CreateRef<Material>();
			archive(cereal::make_nvp("Material", *m_Material.get()));
		}

	private:
		Ref<Material> m_Material;
		bool m_Active = true;
		bool m_TexturesUpdated = false;
	};
}
