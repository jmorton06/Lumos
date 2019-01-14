#include "JM.h"
#include "SkyDomeCamera.h"

namespace jm
{

	SkyDomeCamera::SkyDomeCamera(float Near, float Far)
		: Camera(90.0f, Near, Far, 1.0f)
	{
		m_ProjMatrix = maths::Matrix4::Perspective(m_Near, m_Far, 1.0f, 90.0f);
	}

	SkyDomeCamera::~SkyDomeCamera()
	{
	}

	void SkyDomeCamera::SwitchView(int id)
	{
		switch (id)
		{
		case 0: m_Pitch = 180.0f; m_Yaw = 90.0f; break;
		case 1: m_Pitch = 180.0f; m_Yaw = -90.0f; break;
		case 2: m_Pitch = 90.0f; m_Yaw = 0.0f; break;
		case 3: m_Pitch = -90.0f; m_Yaw = 0.0f; break;
		case 4: m_Pitch = 180.0f; m_Yaw = 0.0f; break;
		case 5: m_Pitch = 180.0f; m_Yaw = 180.0f; break;
		}
	}
}
