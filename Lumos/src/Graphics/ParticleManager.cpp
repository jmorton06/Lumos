#include "LM.h"
#include "ParticleManager.h"
#include "Material.h"
#include "Particle.h"
#include "ParticleEmitter.h"
#include "API/Shader.h"
#include "Camera/Camera.h"
#include "API/Renderer.h"
#include "Mesh.h"
#include "MeshFactory.h"

namespace Lumos
{

	ParticleManager::ParticleManager()
	{
	}

	ParticleManager::~ParticleManager()
	{
	}

	void ParticleManager::Add(Ref<ParticleEmitter> emitter)
	{
		m_Emitters.push_back(emitter);
	}

	void ParticleManager::Render(Camera* camera)
	{
	}

	void ParticleManager::Update(float dt)
	{
		for (auto emitter : m_Emitters)
		{
			emitter->Update(dt);
		}

	}
}
