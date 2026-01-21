#include "LuaDebugPanel.h"
#include "Editor.h"
#include <Lumos/Scripting/Lua/LuaManager.h>
#include <Lumos/Scripting/Lua/LuaScriptComponent.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Scene/Entity.h>
#include <Lumos/Core/Application.h>
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>

#include <imgui/imgui.h>
#include <sol/sol.hpp>
#include <entt/entt.hpp>

namespace Lumos
{
    LuaDebugPanel::LuaDebugPanel()
    {
        m_Name       = ICON_MDI_BUG " Lua Debug###luadebug";
        m_SimpleName = "LuaDebug";
    }

    void LuaDebugPanel::OnNewScene(Scene* scene)
    {
        m_CurrentScene = scene;
    }

    void LuaDebugPanel::OnImGui()
    {
        if(ImGui::Begin(m_Name.c_str(), &m_Active))
        {
            if(m_AutoRefresh)
            {
                m_RefreshTimer += ImGui::GetIO().DeltaTime;
                if(m_RefreshTimer >= m_RefreshInterval)
                {
                    m_RefreshTimer = 0.0f;
                    RefreshGlobals();
                    for(auto& watch : m_WatchList)
                    {
                        watch.Result = EvaluateExpression(watch.Expression);
                        watch.Error = watch.Result.find("Error:") == 0;
                    }
                }
            }

            if(ImGui::BeginTabBar("LuaDebugTabs"))
            {
                if(ImGui::BeginTabItem(ICON_MDI_EYE " Watch"))
                {
                    DrawWatchTab();
                    ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem(ICON_MDI_EARTH " Globals"))
                {
                    DrawGlobalsTab();
                    ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem(ICON_MDI_SCRIPT " Scripts"))
                {
                    DrawScriptsTab();
                    ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem(ICON_MDI_MEMORY " Memory"))
                {
                    DrawMemoryTab();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    void LuaDebugPanel::DrawWatchTab()
    {
        ImGui::Checkbox("Auto Refresh", &m_AutoRefresh);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::SliderFloat("Interval", &m_RefreshInterval, 0.1f, 2.0f, "%.1fs");
        ImGui::SameLine();
        if(ImGui::Button(ICON_MDI_REFRESH " Refresh"))
        {
            for(auto& watch : m_WatchList)
            {
                watch.Result = EvaluateExpression(watch.Expression);
                watch.Error = watch.Result.find("Error:") == 0;
            }
        }

        ImGui::Separator();

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60);
        bool addWatch = ImGui::InputText("##watchinput", m_WatchInput, sizeof(m_WatchInput), ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if(ImGui::Button("Add") || addWatch)
        {
            if(strlen(m_WatchInput) > 0)
            {
                WatchEntry entry;
                entry.Expression = m_WatchInput;
                entry.Result = EvaluateExpression(entry.Expression);
                entry.Error = entry.Result.find("Error:") == 0;
                m_WatchList.push_back(entry);
                m_WatchInput[0] = '\0';
            }
        }

        ImGui::Separator();

        if(ImGui::BeginTable("WatchTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("Expression", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("##del", ImGuiTableColumnFlags_WidthFixed, 30);
            ImGui::TableHeadersRow();

            int toRemove = -1;
            for(int i = 0; i < (int)m_WatchList.size(); i++)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", m_WatchList[i].Expression.c_str());

                ImGui::TableNextColumn();
                if(m_WatchList[i].Error)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                ImGui::TextWrapped("%s", m_WatchList[i].Result.c_str());
                if(m_WatchList[i].Error)
                    ImGui::PopStyleColor();

                ImGui::TableNextColumn();
                ImGui::PushID(i);
                if(ImGui::SmallButton(ICON_MDI_CLOSE))
                    toRemove = i;
                ImGui::PopID();
            }

            if(toRemove >= 0)
                m_WatchList.erase(m_WatchList.begin() + toRemove);

            ImGui::EndTable();
        }
    }

    void LuaDebugPanel::DrawGlobalsTab()
    {
        if(ImGui::Button(ICON_MDI_REFRESH " Refresh"))
            RefreshGlobals();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("Filter", m_GlobalFilter, sizeof(m_GlobalFilter));

        ImGui::Separator();

        if(ImGui::BeginTable("GlobalsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 200);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            std::string filter = m_GlobalFilter;
            for(auto& [name, value] : m_Globals)
            {
                if(!filter.empty() && name.find(filter) == std::string::npos)
                    continue;

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", name.c_str());
                ImGui::TableNextColumn();
                ImGui::TextWrapped("%s", value.c_str());
            }

            ImGui::EndTable();
        }
    }

    void LuaDebugPanel::DrawScriptsTab()
    {
        Scene* scene = m_CurrentScene ? m_CurrentScene : Application::Get().GetCurrentScene();
        if(!scene)
        {
            ImGui::TextDisabled("No scene loaded");
            return;
        }

        auto& registry = scene->GetRegistry();
        auto view = registry.view<LuaScriptComponent>();

        int scriptCount = 0;
        for(auto entity : view)
            scriptCount++;

        ImGui::Text("Active Scripts: %d", scriptCount);
        ImGui::Separator();

        if(ImGui::BeginTable("ScriptsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("Entity", ImGuiTableColumnFlags_WidthFixed, 150);
            ImGui::TableSetupColumn("Script", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableHeadersRow();

            for(auto e : view)
            {
                Entity entity(e, scene);
                auto& script = registry.get<LuaScriptComponent>(e);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", entity.GetName().c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", script.GetFilePath().c_str());

                ImGui::TableNextColumn();
                auto& errors = script.GetErrors();
                if(errors.empty() && script.Loaded())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
                    ImGui::Text(ICON_MDI_CHECK " OK");
                    ImGui::PopStyleColor();
                }
                else if(!script.Loaded())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                    ImGui::Text(ICON_MDI_CIRCLE_OUTLINE " Not Loaded");
                    ImGui::PopStyleColor();
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                    if(ImGui::SmallButton(ICON_MDI_ALERT " Errors"))
                        ImGui::OpenPopup(("Errors##" + std::to_string((uint64_t)e)).c_str());
                    ImGui::PopStyleColor();

                    if(ImGui::BeginPopup(("Errors##" + std::to_string((uint64_t)e)).c_str()))
                    {
                        for(auto& [line, err] : errors)
                        {
                            ImGui::Text("Line %d: %s", line, err.c_str());
                        }
                        ImGui::EndPopup();
                    }
                }
            }

            ImGui::EndTable();
        }
    }

    void LuaDebugPanel::DrawMemoryTab()
    {
        sol::state& state = LuaManager::Get().GetState();

        lua_State* L = state.lua_state();
        int kb = lua_gc(L, LUA_GCCOUNT, 0);
        int bytes = lua_gc(L, LUA_GCCOUNTB, 0);
        float totalKB = kb + bytes / 1024.0f;

        ImGui::Text("Lua Memory Usage: %.2f KB", totalKB);
        ImGui::Separator();

        if(ImGui::Button(ICON_MDI_BROOM " Collect Garbage"))
        {
            LuaManager::Get().CollectGarbage();
        }

        ImGui::SameLine();
        if(ImGui::Button(ICON_MDI_BROOM " Full GC"))
        {
            lua_gc(L, LUA_GCCOLLECT, 0);
        }

        ImGui::Separator();
        ImGui::Text("GC State:");

        int gcRunning = lua_gc(L, LUA_GCISRUNNING, 0);
        ImGui::BulletText("GC Running: %s", gcRunning ? "Yes" : "No");
    }

    std::string LuaDebugPanel::EvaluateExpression(const std::string& expr)
    {
        try
        {
            sol::state& state = LuaManager::Get().GetState();

            std::string evalScript = "return " + expr;
            auto result = state.script(evalScript, sol::script_pass_on_error);

            if(!result.valid())
            {
                sol::error err = result;
                return "Error: " + std::string(err.what());
            }

            sol::object obj = result;
            if(obj.is<std::string>())
                return "\"" + obj.as<std::string>() + "\"";
            else if(obj.is<double>())
                return std::to_string(obj.as<double>());
            else if(obj.is<int>())
                return std::to_string(obj.as<int>());
            else if(obj.is<bool>())
                return obj.as<bool>() ? "true" : "false";
            //else if(obj.is<sol::null>())
              //  return "nil";
            else if(obj.is<sol::table>())
            {
                sol::table tbl = obj;
                std::string str = "{ ";
                int count = 0;
                for(auto& [k, v] : tbl)
                {
                    if(count > 5)
                    {
                        str += "... ";
                        break;
                    }
                    str += "[";
                    if(k.is<std::string>())
                        str += k.as<std::string>();
                    else if(k.is<int>())
                        str += std::to_string(k.as<int>());
                    str += "]=";

                    if(v.is<std::string>())
                        str += "\"" + v.as<std::string>() + "\"";
                    else if(v.is<double>())
                        str += std::to_string(v.as<double>());
                    else if(v.is<bool>())
                        str += v.as<bool>() ? "true" : "false";
                    else
                        str += "[...]";
                    str += ", ";
                    count++;
                }
                str += "}";
                return str;
            }
            else if(obj.is<sol::function>())
                return "[function]";
            else if(obj.is<sol::userdata>())
                return "[userdata]";
            else
                return "[unknown]";
        }
        catch(const std::exception& e)
        {
            return "Error: " + std::string(e.what());
        }
    }

    void LuaDebugPanel::RefreshGlobals()
    {
        m_Globals.clear();

        try
        {
            sol::state& state = LuaManager::Get().GetState();
            sol::table globals = state.globals();

            static const std::vector<std::string> builtins = {
                "_G", "_VERSION", "assert", "collectgarbage", "dofile", "error",
                "getmetatable", "ipairs", "load", "loadfile", "next", "pairs",
                "pcall", "print", "rawequal", "rawget", "rawlen", "rawset",
                "require", "select", "setmetatable", "tonumber", "tostring",
                "type", "xpcall", "coroutine", "debug", "io", "math", "os",
                "package", "string", "table", "utf8"
            };

            for(auto& [key, value] : globals)
            {
                if(!key.is<std::string>())
                    continue;

                std::string name = key.as<std::string>();

                bool isBuiltin = std::find(builtins.begin(), builtins.end(), name) != builtins.end();
                if(isBuiltin)
                    continue;

                std::string valStr;
                if(value.is<std::string>())
                    valStr = "\"" + value.as<std::string>() + "\"";
                else if(value.is<double>())
                    valStr = std::to_string(value.as<double>());
                else if(value.is<int>())
                    valStr = std::to_string(value.as<int>());
                else if(value.is<bool>())
                    valStr = value.as<bool>() ? "true" : "false";
                // else if(value.is<sol::nil_t>())
                  //  valStr = "nil";
                else if(value.is<sol::table>())
                    valStr = "[table]";
                else if(value.is<sol::function>())
                    valStr = "[function]";
                else if(value.is<sol::userdata>())
                    valStr = "[userdata]";
                else
                    valStr = "[unknown]";

                m_Globals.push_back({name, valStr});
            }

            std::sort(m_Globals.begin(), m_Globals.end());
        }
        catch(...)
        {
        }
    }
}
