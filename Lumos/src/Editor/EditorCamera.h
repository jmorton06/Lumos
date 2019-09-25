#pragma once
#include "Graphics/Camera/Camera.h"

namespace Lumos
{
	class Entity;
	class LUMOS_EXPORT EditorCamera : public Camera
	{
	public:
		EditorCamera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect);
		~EditorCamera();

		virtual void HandleMouse(float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(float dt) override;

		void SetSelectedEntity(Entity* entity) { m_SelectedEntity = entity; }

		void SetOrthographic(bool orth) { m_Orthographic = orth; }
		void Set2D(bool is2D) { m_2D = is2D; }


	private:
		Entity* m_SelectedEntity = nullptr;

		bool m_Orthographic = false;
		bool m_2D = false;
	};
}
