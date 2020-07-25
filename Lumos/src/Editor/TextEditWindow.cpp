#include "lmpch.h"
#include "TextEditWindow.h"
#include "Core/OS/FileSystem.h"
#include "Editor.h"
#include "Core/OS/Input.h"

#include <imgui/imgui.h>

namespace Lumos
{
	TextEditWindow::TextEditWindow(const std::string& filePath)
		: m_FilePath(filePath)
	{
		m_Name = "TextEditWindow";
		m_SimpleName = "TextEdit";

		auto extension = StringFormat::GetFilePathExtension(m_FilePath);

		if(extension == "lua" || extension == "Lua")
		{
			auto lang = TextEditor::LanguageDefinition::Lua();
			editor.SetLanguageDefinition(lang);
		}
		else if(extension == "cpp")
		{
			auto lang = TextEditor::LanguageDefinition::CPlusPlus();
			editor.SetLanguageDefinition(lang);
		}
		else if(extension == "glsl" || extension == "vert" || extension == "frag")
		{
			auto lang = TextEditor::LanguageDefinition::GLSL();
			editor.SetLanguageDefinition(lang);
		}

		auto string = FileSystem::ReadTextFile(m_FilePath);
		editor.SetText(string);
	}

	void TextEditWindow::OnImGui()
	{
		auto cpos = editor.GetCursorPosition();
		ImGui::Begin("Text Editor", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
		ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
		if(ImGui::BeginMenuBar())
		{
			if(ImGui::BeginMenu("File"))
			{
				if(ImGui::MenuItem("Save"))
				{
					auto textToSave = editor.GetText();
					FileSystem::WriteTextFile(m_FilePath, textToSave);
				}
				ImGui::EndMenu();
			}
			if(ImGui::BeginMenu("Edit"))
			{
				bool ro = editor.IsReadOnly();
				if(ImGui::MenuItem("Read-only mode", nullptr, &ro))
					editor.SetReadOnly(ro);
				ImGui::Separator();

				if(ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
					editor.Undo();
				if(ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
					editor.Redo();

				ImGui::Separator();

				if(ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
					editor.Copy();
				if(ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
					editor.Cut();
				if(ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
					editor.Delete();
				if(ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
					editor.Paste();

				ImGui::Separator();

				if(ImGui::MenuItem("Select all", nullptr, nullptr))
					editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

				if(ImGui::MenuItem("Close", nullptr, nullptr))
					OnClose();

				ImGui::EndMenu();
			}

			if(ImGui::BeginMenu("View"))
			{
				if(ImGui::MenuItem("Dark palette"))
					editor.SetPalette(TextEditor::GetDarkPalette());
				if(ImGui::MenuItem("Light palette"))
					editor.SetPalette(TextEditor::GetLightPalette());
				if(ImGui::MenuItem("Retro blue palette"))
					editor.SetPalette(TextEditor::GetRetroBluePalette());
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(), editor.IsOverwrite() ? "Ovr" : "Ins", editor.CanUndo() ? "*" : " ", editor.GetLanguageDefinition().mName.c_str(), m_FilePath.c_str());

		if(ImGui::IsItemActive())
		{
			if(Input::GetInput()->GetKeyHeld(InputCode::Key::LeftControl) && Input::GetInput()->GetKeyPressed(InputCode::Key::S))
			{
				auto textToSave = editor.GetText();
				FileSystem::WriteTextFile(m_FilePath, textToSave);
			}
		}

		editor.Render("TextEditor");
		ImGui::End();
	}

	void TextEditWindow::OnClose()
	{
		m_Editor->RemoveWindow(this);
	}
}
