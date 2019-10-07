#pragma once
#include "lmpch.h"
#include "EditorWindow.h"

namespace Lumos
{
	class GraphicsInfoWindow : public EditorWindow
	{
	public:
		GraphicsInfoWindow();
		~GraphicsInfoWindow() = default;

		void OnImGui() override;
	};
}