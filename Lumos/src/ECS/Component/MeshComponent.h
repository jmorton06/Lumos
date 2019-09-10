#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "Graphics/Mesh.h"

namespace Lumos
{
	class LUMOS_EXPORT MeshComponent : public LumosComponent
	{
	public:
        MeshComponent();
		explicit MeshComponent(const Ref<Graphics::Mesh>& mesh);
		explicit MeshComponent(Graphics::Mesh* mesh);
		~MeshComponent() = default;

		void OnImGui() override;

		Graphics::Mesh* GetMesh() const { return m_Mesh.get(); }

		nlohmann::json Serialise() override { return nullptr; };
		void Deserialise(nlohmann::json& data) override {};
	private:
		Ref<Graphics::Mesh> m_Mesh;
	};
}
