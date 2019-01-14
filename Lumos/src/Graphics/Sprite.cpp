#include "LM.h"
#include "Sprite.h"
#include "Graphics/Material.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Mesh.h"
#include "Utilities/AssetsManager.h"
#include "MeshFactory.h"

namespace Lumos
{
	Sprite::Sprite(const maths::Vector2& position, const maths::Vector2& scale, const maths::Vector4& colour, float colourMix)
		: m_Texture(nullptr)
	{
		m_Position = position;
		m_Scale = scale;
		m_Colour = colour;
		m_ColourMix = colourMix;
		m_RotationMatrix.ToIdentity();

		m_TranslationMatrix = maths::Matrix4::Translation(maths::Vector3(position.GetX(), position.GetY(), 1.0f)) * maths::Matrix4::Scale(
				maths::Vector3(scale.GetX(), scale.GetY(), 1.0f));

		m_Mesh = MeshFactory::CreateQuad();

		m_Material = new Material();//std::shared_ptr<Shader>(Shader::CreateFromFile("Sprite", "/Shaders/Scene/Sprite")));
	}

	Sprite::Sprite(std::shared_ptr<Texture2D> texture, const maths::Vector2& position, const maths::Vector2& scale, const maths::Vector4& colour, float colourMix)
	{
		m_Texture = texture;
		m_Position = position;
		m_Scale = scale;
		m_Colour = colour;
		m_ColourMix = colourMix;
		m_RotationMatrix.ToIdentity();

		m_Material = new Material();//std::shared_ptr<Shader>(Shader::CreateFromFile("Sprite", "/Shaders/Scene/Sprite")));
		m_TranslationMatrix = maths::Matrix4::Translation(maths::Vector3(position.GetX(), position.GetY(), 1.0f)) * maths::Matrix4::Scale(maths::Vector3(scale.GetX(), scale.GetY(), 1.0f));
		m_Mesh = MeshFactory::CreateQuad();
	}

	Sprite::~Sprite()
	{
		delete m_Mesh;
		delete m_Material;
	}

	void Sprite::Render(const maths::Matrix4& projMatrix, const maths::Matrix4& viewMatrix) const
	{
		maths::Matrix4 modelMatrix = maths::Matrix4::Translation(maths::Vector3(m_Position, 1.0f));
		modelMatrix = modelMatrix * m_RotationMatrix;
		modelMatrix = modelMatrix * maths::Matrix4::Scale(maths::Vector3(m_Scale, 1.0f));

		//m_Material->SetUniform("modelMatrix", modelMatrix);
		//m_Material->SetUniform("viewMatrix", viewMatrix);
		//m_Material->SetUniform("projMatrix", projMatrix);
		//m_Material->SetUniform("colourMix", m_ColourMix);
		//m_Material->SetUniform("colour", m_Colour);
		//m_Material->SetTexture("diffuseTex", m_Texture.get());

		//m_Material->Bind();
		m_Mesh->Draw();
	}
}
