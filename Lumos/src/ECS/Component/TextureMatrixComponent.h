#pragma once
#include "lmpch.h"
#include "Maths/Maths.h"


namespace Lumos
{
	class LUMOS_EXPORT TextureMatrixComponent
	{
	public:
		explicit TextureMatrixComponent(const Maths::Matrix4& matrix = Maths::Matrix4());

		void OnImGui();
        
        const Maths::Matrix4& GetMatrix() const { return m_TextureMatrix; }

    private:
        Maths::Matrix4 m_TextureMatrix;
	};
}
