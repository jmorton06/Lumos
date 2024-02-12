#include "TextEditPanel.h"
#include "Editor.h"
#include <Lumos/Core/OS/FileSystem.h>
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Utilities/StringUtilities.h>
#include <Lumos/Scripting/Lua/LuaManager.h>

#include <imgui/imgui.h>

namespace Lumos
{
	static bool JustOpenedFile = false;
    TextEditPanel::TextEditPanel(const std::string& filePath)
        : m_FilePath(filePath)
    {
        m_Name           = "Text Editor###textEdit";
		m_ChangedName    = "Text Editor *###textEdit";
        m_SimpleName     = "TextEdit";
        m_OnSaveCallback = NULL;
		m_TextUnsaved    = false;
        editor.SetCustomIdentifiers({});

        auto extension = StringUtilities::GetFilePathExtension(m_FilePath);

        if(extension == "lua" || extension == "Lua")
        {
            auto lang = TextEditor::LanguageDefinition::Lua();
            editor.SetLanguageDefinition(lang);

            auto& customIdentifiers = LuaManager::GetIdentifiers();
            TextEditor::Identifiers identifiers;

            for(auto& k : customIdentifiers)
            {
                TextEditor::Identifier id;
                id.mDeclaration = "Engine function";
                identifiers.insert(std::make_pair(k, id));
            }

            editor.SetCustomIdentifiers(identifiers);
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
        editor.SetShowWhitespaces(false);
		JustOpenedFile = true;
    }

    void TextEditPanel::SetErrors(const std::map<int, std::string>& errors)
    {
        editor.SetErrorMarkers(errors);
    }

    void TextEditPanel::OnImGui()
    {
        auto cpos = editor.GetCursorPosition();
        ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
		std::string& windowName = m_TextUnsaved ? m_ChangedName : m_Name;
        if(ImGui::Begin(windowName.c_str(), &m_Active, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar))
        {
            if(ImGui::BeginMenuBar())
            {
                if(ImGui::BeginMenu("File"))
                {
                    if(ImGui::MenuItem("Save", "CTRL+S"))
                    {
                        auto textToSave = editor.GetText();
                        FileSystem::WriteTextFile(m_FilePath, textToSave);
                        if(m_OnSaveCallback)
                            m_OnSaveCallback();

						m_TextUnsaved = false;
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
                    {
                        OnClose();
                        ImGui::EndMenu();
                        ImGui::EndMenuBar();
                        ImGui::End();
                        return;
                    }

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

            ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(), editor.IsOverwrite() ? "Ovr" : "Ins", editor.CanUndo() ? "*" : " ", editor.GetLanguageDefinition().mName.c_str(), Lumos::StringUtilities::GetFileName(m_FilePath).c_str());

			if(editor.IsTextChanged() && !JustOpenedFile)
				m_TextUnsaved = true;

            editor.Render(m_Name.c_str());

			if(ImGui::IsWindowFocused(ImGuiHoveredFlags_ChildWindows))
			{
				if((Input::Get().GetKeyHeld(InputCode::Key::LeftSuper) || Input::Get().GetKeyHeld(InputCode::Key::LeftControl)) && 
					Input::Get().GetKeyPressed(InputCode::Key::S))
				{
					auto textToSave = editor.GetText();
					FileSystem::WriteTextFile(m_FilePath, textToSave);
					if(m_OnSaveCallback)
						m_OnSaveCallback();

					m_TextUnsaved = false;
				}
			}
			JustOpenedFile = false;

        }
        ImGui::End();
    }

    void TextEditPanel::OnClose()
    {
        m_Editor->RemovePanel(this);
    }
}
