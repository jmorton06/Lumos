#pragma once
#include "lmpch.h"
#include "EditorWindow.h"

namespace Lumos
{
	class AssetWindow : public EditorWindow
	{
	public:
		AssetWindow();
		~AssetWindow() = default;

		void OnImGui() override;
		
	private:

	};
}
