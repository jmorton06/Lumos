#pragma once
#include "lmpch.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"

#include <jsonhpp/json.hpp>

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

		Graphics::Mesh* GetMesh() const { return m_Mesh.get(); }
        void SetMesh(Graphics::Mesh* mesh) { m_Mesh = mesh; }

		bool& GetActive() { return m_Mesh->GetActive(); }

		nlohmann::json Serialise() { return nullptr; };
		void Deserialise(nlohmann::json& data) {};
        
        const String& GetFilePath() const { return m_FilePath; }
        const Graphics::PrimitiveType& GetPrimitiveType() const { return m_PrimitiveType; }

	private:
		Ref<Graphics::Mesh> m_Mesh;

        Graphics::PrimitiveType m_PrimitiveType;
        String m_FilePath;
	};
}
