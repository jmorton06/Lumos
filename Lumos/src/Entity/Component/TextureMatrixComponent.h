#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "Maths/Matrix4.h"

namespace Lumos
{
	class LUMOS_EXPORT TextureMatrixComponent : public LumosComponent
	{
	public:
		maths::Matrix4 m_TextureMatrix;
	public:
		explicit TextureMatrixComponent(const maths::Matrix4& matrix);

		static ComponentType GetStaticType()
		{
			static ComponentType type(ComponentType::TextureMatrix);
			return type;
		}

		inline virtual ComponentType GetType() const override { return GetStaticType(); }
	};
}
