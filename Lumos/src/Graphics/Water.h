#pragma once
#include "lmpch.h"
#include "Mesh.h"

namespace Lumos
{
	class LUMOS_EXPORT Water : public Graphics::Mesh
	{
	public:
		Water();
		~Water();
        
        Water(Water const&) = delete;
        Water& operator=(Water const&) = delete;

		void Draw() override;

	private:

	};
}
