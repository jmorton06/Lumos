#include "lmpch.h"
#include "SpriteComponent.h"
#include "Physics2DComponent.h"
#include "ECS/EntityManager.h"
#include "Physics/B2PhysicsEngine/PhysicsObject2D.h"
#include "Maths/MathsUtilities.h"
#include <imgui/imgui.h>

namespace Lumos
{
    SpriteComponent::SpriteComponent()
    {
        m_Sprite = Ref<Graphics::Sprite>();
    }
    
	SpriteComponent::SpriteComponent(Ref<Graphics::Sprite>& sprite)
		: m_Sprite(sprite)
	{
	}

	void SpriteComponent::OnImGui()
	{

	}

}
