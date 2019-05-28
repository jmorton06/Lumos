#pragma once
#include "LM.h"
#include "Maths/Maths.h"

namespace lumos
{
	class ParticleEmitter;
	class Camera;

	class LUMOS_EXPORT ParticleManager
	{
	public:

		ParticleManager();
		~ParticleManager();

        void Add(std::shared_ptr<ParticleEmitter> emitter);
		void Render(Camera* camera);

		void Update(float dt);

	private:

		std::vector<std::shared_ptr<ParticleEmitter>> m_Emitters;
	};
}
