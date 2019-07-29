#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "Maths/Matrix4.h"

namespace Lumos
{
	class LUMOS_EXPORT TextureMatrixComponent : public LumosComponent
	{
	public:
		explicit TextureMatrixComponent(const Maths::Matrix4& matrix);

		void OnIMGUI() override;
        
        const Maths::Matrix4& GetMatrix() const { return m_TextureMatrix; }

		nlohmann::json Serialise() override { return nullptr; };
		void Deserialise(nlohmann::json& data) override {};

    private:
        Maths::Matrix4 m_TextureMatrix;
	};
}
