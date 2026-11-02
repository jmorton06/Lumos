#include "LuaConsolePanel.h"
#include <Lumos/Core/Thread.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Scene/Scene.h>
#include <entt/entity/registry.hpp>
#include <Lumos/Graphics/UI.h>
#include <Lumos/Graphics/UIDock.h>
#include <Lumos/Graphics/UITheme.h>
#include <Lumos/Maths/MathsUtilities.h>
#include <Lumos/Graphics/UICodeEditor.h>
#include <Lumos/Scripting/Lua/LuaManager.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <sol/sol.hpp>

namespace Lumos
{
    namespace
    {
        struct Example
        {
            const char* Name;
            const char* Description;
            const char* Code;
        };

        const Example kExamples[] = {
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
        constexpr int kExampleCount = sizeof(kExamples) / sizeof(kExamples[0]);

        // output ring: fixed lines, no allocation per print
        constexpr u32 kLines    = 256;
        constexpr u32 kLineSize = 240;
        struct Line
        {
            char Text[kLineSize];
            u8 Kind; // 0 text, 1 error, 2 done
        };
        Line s_Lines[kLines];
        u32 s_Head  = 0;
        u32 s_Count = 0;
        f32 s_Scroll = 0.0f;
        bool s_FollowOutput = true;
        UICodeEditor* s_Code = nullptr;

        void Push(const char* text, u8 kind)
        {
            Line& line = s_Lines[(s_Head + s_Count) % kLines];
            snprintf(line.Text, sizeof(line.Text), "%s", text);
            line.Kind = kind;
            if(s_Count < kLines)
                s_Count++;
            else
                s_Head = (s_Head + 1) % kLines;
            s_FollowOutput = true;
        }

        void Run()
        {
            String8 text = UICodeEditorGetText(s_Code);
            if(text.size == 0)
                return;
            sol::state& lua = LuaManager::Get().GetState();
            sol::environment env(lua, sol::create, lua.globals());
            if(Scene* scene = Application::Get().GetCurrentScene())
            {
                env["scene"]    = scene;
                env["registry"] = &scene->GetRegistry();
            }
            env["print"] = [](sol::variadic_args args, sol::this_state state)
            {
                sol::state_view view(state);
                sol::protected_function tostring = view["tostring"];
                char out[kLineSize];
                u32 at = 0;
                out[0] = 0;
                for(auto arg : args)
                {
                    sol::protected_function_result piece = tostring(arg.get<sol::object>());
                    const char* str = piece.valid() ? piece.get<const char*>() : "?";
                    at += snprintf(out + at, at < sizeof(out) ? sizeof(out) - at : 0, at ? "\t%s" : "%s", str ? str : "nil");
                    if(at >= sizeof(out))
                        break;
                }
                Push(out, 0);
            };

            ArenaTemp scratch = ScratchBegin(0, 0);
            String8 code      = PushStr8Copy(scratch.arena, text); // null-terminated for the parser
            sol::protected_function_result result = lua.safe_script(std::string_view((const char*)code.str, code.size), env, sol::script_pass_on_error);
            ScratchEnd(scratch);
            if(!result.valid())
            {
                sol::error error = result;
                Push(error.what(), 1);
                return;
            }
            if(result.return_count() > 0 && result.get_type() != sol::type::lua_nil)
            {
                sol::protected_function tostring = lua["tostring"];
                sol::protected_function_result value = tostring(result.get<sol::object>());
                if(value.valid())
                {
                    char line[kLineSize];
                    snprintf(line, sizeof(line), "-> %s", value.get<const char*>());
                    Push(line, 0);
                }
            }
            Push("Done", 2);
        }
    }

    void LuaConsolePanel::Release()
    {
        if(s_Code)
            UICodeEditorDestroy(s_Code);
        s_Code = nullptr;
    }

    void LuaConsolePanel::Draw(const UIDockPanelContext& context)
    {
        if(!s_Code)
        {
            s_Code = UICodeEditorCreate();
            UICodeEditorSetLanguage(s_Code, UICodeLanguage_Lua);
            UICodeEditorSetText(s_Code, Str8C((char*)kExamples[0].Code));
        }
        const UIToolTheme& theme = UIGetToolTheme();
        const f32 dpi            = context.DPIScale;
        Input& input             = Input::Get();

        UIBeginRow();
        if(UIButton(ICON_MDI_PLAY " Run###luarun").clicked)
            Run();
        UITooltip("Run (Ctrl Enter)");
        if(UIButton(ICON_MDI_DELETE " Clear###luaclear").clicked)
            s_Count = s_Head = 0;
        static int s_Example = 0;
        const char* names[kExampleCount];
        for(int i = 0; i < kExampleCount; i++)
            names[i] = kExamples[i].Name;
        const int before = s_Example;
        UIDropdown("Examples###luaexamples", &s_Example, names, kExampleCount);
        if(s_Example != before)
            UICodeEditorSetText(s_Code, Str8C((char*)kExamples[s_Example].Code));
        UIEndRow();

        const f32 bar    = 34.0f * dpi;
        const f32 height = Maths::Max(80.0f * dpi, context.Size.y - bar);
        const f32 editor = floorf(height * 0.62f);
        UICodeEditorDraw("##luaconsolecode", s_Code, Vec2(context.Size.x, editor));

        const bool mod = input.GetKeyHeldRaw(InputCode::Key::LeftControl) || input.GetKeyHeldRaw(InputCode::Key::LeftSuper)
            || input.GetKeyHeldRaw(InputCode::Key::RightControl) || input.GetKeyHeldRaw(InputCode::Key::RightSuper);
        if(context.Focused && mod && input.GetKeyPressedRaw(InputCode::Key::Enter))
            Run();

        // one line a row (long ones run off the side), so the newest can be kept in view
        const f32 outH = Maths::Max(40.0f * dpi, height - editor - 8.0f * dpi);
        const f32 row  = (theme.FontSize + 4.0f) * dpi + 2.0f * dpi;
        if(s_FollowOutput)
            s_Scroll = Maths::Max(0.0f, (f32)s_Count * row - outH);
        s_FollowOutput = false;
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 2.0f * dpi));
        UIBeginScrollArea("##luaconsoleout", outH, &s_Scroll);
        char id[24];
        for(u32 i = 0; i < s_Count; i++)
        {
            const Line& line = s_Lines[(s_Head + i) % kLines];
            snprintf(id, sizeof(id), "##luaout%u", i);
            const Vec4 colour = line.Kind == 1 ? Vec4(1.0f, 0.45f, 0.4f, 1.0f) : (line.Kind == 2 ? theme.TextDim : theme.Text);
            UIPushStyle(StyleVar_TextColor, colour);
            UILabel(id, Str8C((char*)line.Text));
            UIPopStyle(StyleVar_TextColor);
        }
        UIEndScrollArea();
        UIPopStyle(StyleVar_ItemSpacing);
    }
}
