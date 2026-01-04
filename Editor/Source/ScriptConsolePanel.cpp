#include "ScriptConsolePanel.h"
#include "Editor.h"
#include <Lumos/Scripting/Lua/LuaManager.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Core/OS/Input.h>

#include <imgui/imgui.h>
#include <sol/sol.hpp>

namespace Lumos
{
    ScriptConsolePanel::ScriptConsolePanel()
    {
        m_Name       = "Script Console###scriptconsole";
        m_SimpleName = "ScriptConsole";

        auto lang = TextEditor::LanguageDefinition::Lua();
        m_Editor.SetLanguageDefinition(lang);

        auto& customIdentifiers = LuaManager::GetIdentifiers();
        TextEditor::Identifiers identifiers;

        for(auto& k : customIdentifiers)
        {
            TextEditor::Identifier id;
            id.mDeclaration = "Engine function";
            identifiers.insert(std::make_pair(k, id));
        }

        m_Editor.SetCustomIdentifiers(identifiers);
        m_Editor.SetShowWhitespaces(false);
        m_Editor.SetText("-- Enter Lua script here\n-- Use 'scene' to access current scene\n-- Use 'registry' to access ECS registry\n\n");
    }

    void ScriptConsolePanel::OnNewScene(Scene* scene)
    {
        m_CurrentScene = scene;
    }

    void ScriptConsolePanel::OnImGui()
    {
        ImGui::SetWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

        if(ImGui::Begin(m_Name.c_str(), &m_Active, ImGuiWindowFlags_MenuBar))
        {
            if(ImGui::BeginMenuBar())
            {
                if(ImGui::BeginMenu("Script"))
                {
                    if(ImGui::MenuItem("Run", "Ctrl+Enter"))
                    {
                        ExecuteScript();
                    }
                    if(ImGui::MenuItem("Clear Output"))
                    {
                        ClearOutput();
                    }
                    if(ImGui::MenuItem("Clear Script"))
                    {
                        m_Editor.SetText("");
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            if(ImGui::Button("Run Script"))
            {
                ExecuteScript();
            }
            ImGui::SameLine();
            if(ImGui::Button("Clear Output"))
            {
                ClearOutput();
            }
            ImGui::SameLine();
            ImGui::Checkbox("Auto-scroll", &m_AutoScroll);

            ImGui::Separator();

            float availableHeight = ImGui::GetContentRegionAvail().y;
            float editorHeight    = availableHeight * 0.6f;
            float outputHeight    = availableHeight * 0.4f - ImGui::GetFrameHeightWithSpacing();

            ImGui::BeginChild("ScriptEditor", ImVec2(0, editorHeight), true);
            m_Editor.Render("ScriptEditor");
            ImGui::EndChild();

            if(ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
            {
                bool ctrlHeld = Input::Get().GetKeyHeld(InputCode::Key::LeftControl) || Input::Get().GetKeyHeld(InputCode::Key::LeftSuper);
                if(ctrlHeld && Input::Get().GetKeyPressed(InputCode::Key::Enter))
                {
                    ExecuteScript();
                }
            }

            ImGui::Separator();
            ImGui::Text("Output:");

            ImGui::BeginChild("OutputArea", ImVec2(0, outputHeight), true);
            for(const auto& line : m_OutputHistory)
            {
                // Color errors red
                if(line.find("Error:") != std::string::npos || line.find("error") != std::string::npos)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                    ImGui::TextWrapped("%s", line.c_str());
                    ImGui::PopStyleColor();
                }
                else if(line.find("Success") != std::string::npos)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
                    ImGui::TextWrapped("%s", line.c_str());
                    ImGui::PopStyleColor();
                }
                else
                {
                    ImGui::TextWrapped("%s", line.c_str());
                }
            }

            if(m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);

            ImGui::EndChild();
        }
        ImGui::End();
    }

    void ScriptConsolePanel::ExecuteScript()
    {
        std::string script = m_Editor.GetText();

        if(script.empty())
            return;

        m_OutputHistory.push_back(">>> Executing script...");

        try
        {
            sol::state& state = LuaManager::Get().GetState();

            sol::environment env(state, sol::create, state.globals());

            Scene* scene = m_CurrentScene ? m_CurrentScene : Application::Get().GetCurrentScene();
            if(scene)
            {
                env["scene"]    = scene;
                env["registry"] = &scene->GetRegistry();
            }

            // Custom print function that captures output
            env["print"] = [this](sol::variadic_args args) {
                std::string output;
                for(auto arg : args)
                {
                    sol::object obj = arg;
                    if(obj.is<std::string>())
                        output += obj.as<std::string>();
                    else if(obj.is<double>())
                        output += std::to_string(obj.as<double>());
                    else if(obj.is<int>())
                        output += std::to_string(obj.as<int>());
                    else if(obj.is<bool>())
                        output += obj.as<bool>() ? "true" : "false";
                    else
                        output += "[object]";
                    output += "\t";
                }
                m_OutputHistory.push_back(output);
            };

            auto result = state.script(script, env, sol::script_pass_on_error);

            if(!result.valid())
            {
                sol::error err = result;
                m_OutputHistory.push_back("Error: " + std::string(err.what()));
            }
            else
            {
                // If the script returns a value, print it
                if(result.return_count() > 0)
                {
                    sol::object retVal = result;
                    if(retVal.is<std::string>())
                        m_OutputHistory.push_back("Return: " + retVal.as<std::string>());
                    else if(retVal.is<double>())
                        m_OutputHistory.push_back("Return: " + std::to_string(retVal.as<double>()));
                    else if(retVal.is<int>())
                        m_OutputHistory.push_back("Return: " + std::to_string(retVal.as<int>()));
                    else if(retVal.is<bool>())
                        m_OutputHistory.push_back("Return: " + std::string(retVal.as<bool>() ? "true" : "false"));
                }
                m_OutputHistory.push_back("Success: Script executed.");
            }
        }
        catch(const std::exception& e)
        {
            m_OutputHistory.push_back("Error: " + std::string(e.what()));
        }
    }

    void ScriptConsolePanel::ClearOutput()
    {
        m_OutputHistory.clear();
    }
}
