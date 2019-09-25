#pragma once
#include "lmpch.h"
#include "Maths/Maths.h"
#include "ParticleEmitter.h"

namespace Lumos
{
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
