#include "LM.h"
#include "SpriteComponent.h"
#include "Graphics/Sprite.h"
#include "Physics2DComponent.h"
#include "Entity/Entity.h"
#include "Physics/B2PhysicsEngine/PhysicsObject2D.h"
#include "Maths/MathsUtilities.h"
#include <imgui/imgui.h>

namespace Lumos
{
    SpriteComponent::SpriteComponent()
    {
        m_Sprite = std::shared_ptr<Graphics::Sprite>();
        m_Name = "Sprite";
    }
    
	SpriteComponent::SpriteComponent(std::shared_ptr<Graphics::Sprite>& sprite)
		: m_Sprite(sprite)
	{
		m_Name = "Sprite";
	}

	void SpriteComponent::OnUpdateComponent(float dt)
	{
	}

	void SpriteComponent::OnIMGUI()
	{

	}

}
