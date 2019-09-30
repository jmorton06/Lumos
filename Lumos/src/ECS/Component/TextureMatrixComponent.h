#pragma once
#include "lmpch.h"
#include "Maths/Matrix4.h"

namespace Lumos
{
	class LUMOS_EXPORT TextureMatrixComponent
	{
	public:
		explicit TextureMatrixComponent(const Maths::Matrix4& matrix = Maths::Matrix4());

		void OnImGui();
        
        const Maths::Matrix4& GetMatrix() const { return m_TextureMatrix; }

		nlohmann::json Serialise();
		void Deserialise(nlohmann::json& data);

    private:
        Maths::Matrix4 m_TextureMatrix;
	};
}
