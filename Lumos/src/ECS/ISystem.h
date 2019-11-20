#pragma once
#include "lmpch.h"
#include "App/Scene.h"

namespace Lumos
{
	class TimeStep;

	class LUMOS_EXPORT ISystem
	{
	public:
		ISystem() = default;
		virtual ~ISystem() = default;

		virtual void OnInit() = 0;
		virtual void OnUpdate(TimeStep* dt, Scene* scene) = 0;
		virtual void OnImGui() = 0;
        
        _FORCE_INLINE_ const String& GetName() const { return m_DebugName; }

    protected:
        String m_DebugName;
	};
}
