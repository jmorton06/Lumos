#pragma once
#include "lmpch.h"
#include "EditorWindow.h"
#include "Core/Profiler.h"

namespace Lumos
{
	class ProfilerWindow : public EditorWindow
	{
	public:
		ProfilerWindow();
		~ProfilerWindow() = default;

		void OnImGui() override;
	private:
		float m_UpdateFrequency;
		float m_UpdateTimer;

		ProfilerReport m_Report;
	};
}