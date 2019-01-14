#pragma once
#include "JM.h"
#include "Maths/Maths.h"
#include "Mesh.h"

namespace jm
{
	class Mesh;
	class Framebuffer;
	class Texture2D;
	class Shader;
	class Material;
	class Timer;

	class JM_EXPORT Water : public Mesh
	{
	public:
		Water(const maths::Vector3 &position, const maths::Vector3 &scale);
		~Water();

		void Draw() override;

	private:

		Timer* m_Timer;
		float  m_WAVE_SPEED = 4.0f;
		float  m_moveFactor = 0.0f;

		std::shared_ptr<Shader> m_Shader;
	};
}
