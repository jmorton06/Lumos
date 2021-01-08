#pragma once

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