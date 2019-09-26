#pragma once
#include "lmpch.h"
#include "EditorWindow.h"

namespace Lumos
{
	class InspectorWindow : public EditorWindow
	{
	public:
		InspectorWindow();
		~InspectorWindow() = default;

		void OnImGui() override;
	};
}