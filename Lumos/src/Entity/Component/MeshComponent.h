#pragma once
#include "LM.h"
#include "LumosComponent.h"

namespace Lumos
{
	class Mesh;

	class LUMOS_EXPORT MeshComponent : public LumosComponent
	{
	public:
		std::shared_ptr<Mesh> m_Model;
	public:
		explicit MeshComponent(std::shared_ptr<Mesh>& model);

		static ComponentType GetStaticType()
		{
			static ComponentType type(ComponentType::Mesh);
			return type;
		}

		void OnUpdateComponent(float dt) override;

		inline virtual ComponentType GetType() const override { return GetStaticType(); }

		void OnIMGUI() override;
	};
}
