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
		m_ParticleMaterial = new Material();
		m_Quad = MeshFactory::CreateQuad();
		m_VAO = VertexArray::Create();

	}

	ParticleManager::~ParticleManager()
	{
		m_Emitters.clear();

		delete m_ParticleMaterial;
        delete m_Quad;
	}

	void ParticleManager::Add(std::shared_ptr<ParticleEmitter> emitter)
	{
		m_Emitters.push_back(emitter);
	}

	void ParticleManager::Render(Camera* camera)
	{
		m_VAO->Bind();

		Renderer::SetDepthMask(false);
		Renderer::SetBlend(true);
		Renderer::SetBlendFunction(RendererBlendFunction::SOURCE_ALPHA, RendererBlendFunction::ONE);

		for (auto emitter : m_Emitters)
		{
			emitter->Render(m_ParticleMaterial, camera, m_Quad);
		}

		Renderer::SetDepthMask(true);
		Renderer::SetBlend(false);

		m_VAO->Unbind();
	}

	void ParticleManager::Update(float dt)
	{
		for (auto emitter : m_Emitters)
		{
			emitter->Update(dt);
		}

	}
}
