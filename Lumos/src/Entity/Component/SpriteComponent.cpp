#include "LM.h"
#include "SpriteComponent.h"
#include "Graphics/Sprite.h"
#include "Physics2DComponent.h"
#include "Entity/Entity.h"
#include "Physics/B2PhysicsEngine/PhysicsObject2D.h"
#include "Maths/MathsUtilities.h"
#include <imgui/imgui.h>

namespace lumos
{
	SpriteComponent::SpriteComponent(std::shared_ptr<Sprite>& sprite)
		: m_Sprite(sprite)
	{

	}

	void SpriteComponent::OnUpdateComponent(float dt)
	{
	}

	void SpriteComponent::OnIMGUI()
	{
		if (ImGui::TreeNode("Sprite"))
		{
			ImGui::TreePop();
		}
	}

}
