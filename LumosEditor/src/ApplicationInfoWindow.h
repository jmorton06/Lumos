#pragma once

#include "EditorWindow.h"

namespace Lumos
{
	class ApplicationInfoWindow : public EditorWindow
	{
	public:
		ApplicationInfoWindow();
		~ApplicationInfoWindow() = default;

		void OnImGui() override;
	};
}