#pragma once
#include "lmpch.h"
#include "Graphics/Mesh.h"

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

		bool& GetActive() { return m_Mesh->GetActive(); }

		nlohmann::json Serialise() { return nullptr; };
		void Deserialise(nlohmann::json& data) {};

	private:
		Ref<Graphics::Mesh> m_Mesh;
	};
}
