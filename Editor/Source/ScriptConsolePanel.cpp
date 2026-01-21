#include "ScriptConsolePanel.h"
#include "Editor.h"
#include <Lumos/Scripting/Lua/LuaManager.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Core/OS/Input.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>

#include <imgui/imgui.h>
#include <sol/sol.hpp>

namespace Lumos
{
    const std::vector<ScriptExample> ScriptConsolePanel::s_Examples = {
        {
            "-- Select Example --",
            "Choose an example script",
            ""
        },
        {
            "List All Entities",
            "Print all entities in scene",
            R"(-- List all entities in scene
local count = 0
for entity in EachEntity() do
    local name = entity:GetName()
    print(count .. ": " .. name)
    count = count + 1
end
print("Total entities: " .. count))"
        },
        {
            "Find Entity By Name",
            "Find and select entity",
            R"(-- Find entity by name
local name = "Player"  -- Change this
local entity = scene:GetEntityByName(name)
if entity:Valid() then
    print("Found: " .. entity:GetName())
else
    print("Entity not found: " .. name)
end)"
        },
        {
            "Create Cube",
            "Spawn a cube entity",
            R"(-- Create a cube entity
local entity = AddCubeEntity("NewCube")
local transform = entity:GetTransform()
transform:SetLocalPosition(Vec3.new(0, 2, 0))
print("Created cube: " .. entity:GetName()))"
        },
        {
            "Create Sphere",
            "Spawn a sphere entity",
            R"(-- Create a sphere entity
local entity = AddSphereEntity("NewSphere")
local transform = entity:GetTransform()
transform:SetLocalPosition(Vec3.new(0, 2, 0))
print("Created sphere: " .. entity:GetName()))"
        },
        {
            "Create Light",
            "Add a point light",
            R"(-- Create a point light
local entity = AddLightCubeEntity("NewLight")
local transform = entity:GetTransform()
transform:SetLocalPosition(Vec3.new(0, 5, 0))
local light = entity:GetLight()
light.Intensity = 2.0
print("Created light: " .. entity:GetName()))"
        },
        {
            "Move All Entities",
            "Translate all entities up",
            R"(-- Move all entities up by 1 unit
for entity in EachEntity() do
    local transform = entity:GetTransform()
    if transform then
        local pos = transform:GetLocalPosition()
        transform:SetLocalPosition(Vec3.new(pos.x, pos.y + 1, pos.z))
    end
end
print("Moved all entities up"))"
        },
        {
            "Random Colors",
            "Randomize sprite colors",
            R"(-- Randomize sprite colors
math.randomseed(os.time())
for entity in EachEntity() do
    local sprite = entity:TryGetSprite()
    if sprite then
        sprite.Colour = Vec4.new(
            math.random(),
            math.random(),
            math.random(),
            1.0
        )
    end
end
print("Randomized sprite colors"))"
        },
        {
            "Scene Stats",
            "Print scene statistics",
            R"(-- Print scene statistics
local entities = 0
local transforms = 0
local sprites = 0
local models = 0
local lights = 0

for entity in EachEntity() do
    entities = entities + 1
    if entity:HasTransform() then transforms = transforms + 1 end
    if entity:HasSprite() then sprites = sprites + 1 end
    if entity:HasModel() then models = models + 1 end
    if entity:HasLight() then lights = lights + 1 end
end

print("=== Scene Stats ===")
print("Entities: " .. entities)
print("Transforms: " .. transforms)
print("Sprites: " .. sprites)
print("Models: " .. models)
print("Lights: " .. lights))"
        },
        {
            "Delete By Name",
            "Delete entities matching name",
            R"(-- Delete entities containing name
local pattern = "Cube"  -- Change this
local deleted = 0
for entity in EachEntity() do
    if string.find(entity:GetName(), pattern) then
        entity:Destroy()
        deleted = deleted + 1
    end
end
print("Deleted " .. deleted .. " entities"))"
        },
        {
            "Gravity Toggle",
            "Toggle physics gravity",
            R"(-- Toggle 2D physics gravity
local current = GetB2DGravity()
if current.y < 0 then
    SetB2DGravity(Vec2.new(0, 0))
    print("Gravity OFF")
else
    SetB2DGravity(Vec2.new(0, -9.8))
    print("Gravity ON")
end)"
        },
        {
            "Input Test",
            "Test keyboard input",
            R"(-- Check input state (run multiple times)
print("=== Input State ===")
print("Space: " .. tostring(Input.GetKeyHeld(Key.Space)))
print("W: " .. tostring(Input.GetKeyHeld(Key.W)))
print("Mouse L: " .. tostring(Input.GetMouseHeld(MouseButton.Left)))
local pos = Input.GetMousePosition()
print("Mouse: " .. pos.x .. ", " .. pos.y))"
        },
        {
            "Timer Utility",
            "Create a timer function",
            R"(-- Define a reusable timer (stores in global)
Timer = Timer or {}
Timer.start = os.clock()

function Timer.elapsed()
    return os.clock() - Timer.start
end

function Timer.reset()
    Timer.start = os.clock()
    print("Timer reset")
end

print("Elapsed: " .. string.format("%.2f", Timer.elapsed()) .. "s")
print("Call Timer.reset() to restart"))"
        },
        {
            "UI Panel Example",
            "Create a simple UI panel",
            R"(-- Create a UI panel (run in game mode)
UIBeginPanel("My Panel", 200, 100, 250, 200)

if UIButton("Click Me", 10, 10, 100, 30) then
    print("Button clicked!")
end

UILabel("Hello World", 10, 50)

local value = UISlider("Speed", 10, 80, 150, 0, 100, 50)
print("Slider: " .. value)

UIEndPanel())"
        }
    };

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

            if(ImGui::Button(ICON_MDI_PLAY " Run"))
            {
                ExecuteScript();
            }
            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_DELETE " Clear"))
            {
                ClearOutput();
            }
            ImGui::SameLine();
            ImGui::Checkbox("Auto-scroll", &m_AutoScroll);

            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            DrawExamplesDropdown();

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

    void ScriptConsolePanel::DrawExamplesDropdown()
    {
        ImGui::SetNextItemWidth(180);
        if(ImGui::BeginCombo("##Examples", ICON_MDI_CODE_TAGS " Examples"))
        {
            for(int i = 1; i < (int)s_Examples.size(); i++)
            {
                bool selected = (m_SelectedExample == i);
                if(ImGui::Selectable(s_Examples[i].Name, selected))
                {
                    m_SelectedExample = i;
                    m_Editor.SetText(s_Examples[i].Code);
                }

                if(ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", s_Examples[i].Description);
                    ImGui::EndTooltip();
                }
            }
            ImGui::EndCombo();
        }

        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Load example scripts");
            ImGui::EndTooltip();
        }
    }
}
