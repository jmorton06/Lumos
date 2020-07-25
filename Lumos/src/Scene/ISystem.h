#pragma once
#include "lmpch.h"
#include "Scene/Scene.h"

namespace Lumos
{
	class TimeStep;

	class LUMOS_EXPORT ISystem
	{
	public:
		ISystem() = default;
		virtual ~ISystem() = default;

		virtual void OnInit() = 0;
		virtual void OnUpdate(const TimeStep& dt, Scene* scene) = 0;
		virtual void OnImGui() = 0;
		virtual void OnDebugDraw() = 0;

		_FORCE_INLINE_ const std::string& GetName() const
		{
			return m_DebugName;
		}

	protected:
		std::string m_DebugName;
	};
}
