#pragma once
#include "lmpch.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"

namespace Lumos
{
	class LUMOS_EXPORT MeshComponent
	{
	public:
		MeshComponent();
		explicit MeshComponent(const Ref<Graphics::Mesh>& mesh);
		explicit MeshComponent(Graphics::Mesh* mesh);
		~MeshComponent() = default;

		void OnImGui();

		Graphics::Mesh* GetMesh() const
		{
			return m_Mesh.get();
		}
		void SetMesh(Graphics::Mesh* mesh)
		{
			m_Mesh = Ref<Graphics::Mesh>(mesh);
		}

		bool& GetActive()
		{
			return m_Mesh->GetActive();
		}

		const std::string& GetFilePath() const
		{
			return m_FilePath;
		}
		const Graphics::PrimitiveType& GetPrimitiveType() const
		{
			return m_PrimitiveType;
		}

		template<class Archive>
		void load(Archive& archive)
		{
			archive(m_FilePath);
			SetMesh(Graphics::CreateSphere());
		}

		template<class Archive>
		void save(Archive& archive) const
		{
			archive(m_FilePath);
			//todo
		}

	private:
		Ref<Graphics::Mesh> m_Mesh;

		Graphics::PrimitiveType m_PrimitiveType;
		std::string m_FilePath;
	};
}
