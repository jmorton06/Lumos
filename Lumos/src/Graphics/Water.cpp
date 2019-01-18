#include "LM.h"
#include "Water.h"
#include "Utilities/AssetsManager.h"
#include "API/Framebuffer.h"
#include "App/SceneManager.h"
#include "Mesh.h"
#include "Material.h"
#include "MeshFactory.h"
#include "App/Application.h"
#include "Utilities/Timer.h"
#include "Graphics/API/Shader.h"

namespace Lumos
{

	Water::Water(const maths::Vector3 &position, const maths::Vector3 &scale)
	{
		auto waterQuad = MeshFactory::CreateQuad();
		m_Shader = std::shared_ptr<Shader>(Shader::CreateFromFile("Water", "/Shaders/Scene/Water"));

		m_Material = std::make_shared<Material>(m_Shader);
		//m_Material->SetTexture("dudvMap", AssetsManager::DefaultTextures()->GetAsset("WaterDUDV").get());
		//m_Material->SetTexture("normalMap", AssetsManager::DefaultTextures()->GetAsset("WaterNormal").get());
		m_IndexBuffer = waterQuad->GetIndexBuffer();
		m_VertexArray = waterQuad->GetVertexArray();

		delete waterQuad;

		m_Timer = new Timer();
	}

	Water::~Water()
	{
		//delete m_Material;
		delete m_Timer;
	}

	void Water::Draw()
	{
		m_moveFactor = static_cast<float>(m_Timer->GetMS()) * m_WAVE_SPEED / 100.0f;

		m_moveFactor = m_moveFactor - static_cast<long>(m_moveFactor);

		//GetMaterial()->SetUniform("moveFactor", m_moveFactor);
		//GetMaterial()->SetTexture("dudvMap", AssetsManager::DefaultTextures()->GetAsset("WaterDUDV").get());
		//GetMaterial()->SetTexture("normalMap", AssetsManager::DefaultTextures()->GetAsset("WaterNormal").get());

		Mesh::Draw();
	}
}
