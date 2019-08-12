#pragma once
#include "LM.h"
#include "Maths/Maths.h"

namespace Lumos
{
	class ParticleEmitter;
	class Camera;

	class LUMOS_EXPORT ParticleManager
	{
	public:

		ParticleManager();
		~ParticleManager();

        void Add(Ref<ParticleEmitter> emitter);
		void Render(Camera* camera);

		void Update(float dt);

	private:

		std::vector<Ref<ParticleEmitter>> m_Emitters;
	};
}
