#pragma once
#include "LM.h"
#include "Maths/Maths.h"

namespace Lumos
{
	class Material;
	class Particle;
	class Camera;
	class Mesh;
	class VertexArray;
	class ParticleEmitter;

	class LUMOS_EXPORT ParticleManager
	{
	public:

		ParticleManager();
		~ParticleManager();

        void Add(std::shared_ptr<ParticleEmitter> emitter);
		void Render(Camera* camera);

		void Update(float dt);

	private:

		Material* m_ParticleMaterial;
		Mesh* m_Quad;
		VertexArray* m_VAO;
		std::vector<std::shared_ptr<ParticleEmitter>> m_Emitters;
	};
}
