#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "Maths/Matrix4.h"

namespace Lumos
{
	class LUMOS_EXPORT TextureMatrixComponent : public LumosComponent
	{
	public:
		Maths::Matrix4 m_TextureMatrix;
	public:
		explicit TextureMatrixComponent(const Maths::Matrix4& matrix);

		void OnIMGUI() override;
	};
}
