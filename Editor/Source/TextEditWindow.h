#pragma once
#include "EditorWindow.h"
#include <imgui/plugins/ImTextEditor.h>

namespace Lumos
{
	class TextEditWindow : public EditorWindow
	{
	public:
		TextEditWindow(const std::string& filePath);
		~TextEditWindow() = default;

		void OnImGui() override;
		void OnClose();

	private:
		std::string m_FilePath;
		TextEditor editor;
	};
}
