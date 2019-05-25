#pragma once
#include "LM.h"
#include "Mesh.h"

namespace lumos
{
	class LUMOS_EXPORT Water : public graphics::Mesh
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
