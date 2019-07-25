#pragma once
#include "LM.h"
#include "LumosComponent.h"

namespace Lumos
{
	namespace Graphics
	{
		class Mesh;
	}

	class LUMOS_EXPORT MeshComponent : public LumosComponent
	{
	public:
        MeshComponent();
		explicit MeshComponent(std::shared_ptr<Graphics::Mesh>& mesh);
		explicit MeshComponent(Graphics::Mesh* mesh);
		~MeshComponent() = default;

		void OnUpdateComponent(float dt) override;

		void OnIMGUI() override;

		Graphics::Mesh* GetMesh() const { return m_Mesh.get(); }
	private:
		std::shared_ptr<Graphics::Mesh> m_Mesh;
	};
}
