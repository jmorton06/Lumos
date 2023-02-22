#pragma once

#include <imgui.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// TODO support std::string_view
// TODO support function pointer callback in addition to std::function

enum ImCmdTextType
{
    ImCmdTextType_Regular,
    ImCmdTextType_Highlight,
    ImCmdTextType_COUNT,
};

enum ImCmdTextFlag
{
    /// Whether the text is underlined. Default false.
    ImCmdTextFlag_Underline,
    ImCmdTextFlag_COUNT,
};

namespace ImCmd
{
struct Command
{
    std::string Name;
    std::function<void()> InitialCallback;
    std::function<void(int selected_option)> SubsequentCallback;
    std::function<void()> TerminatingCallback;
};

// Initialization
struct Context;

/// Create a new context object. If there is currently no context bound, it will also be bound as the current context.
Context* CreateContext();
/// Destroys the currently bound context.
void DestroyContext();
void DestroyContext(Context* context);

void SetCurrentContext(Context* context);
Context* GetCurrentContext();

// Command management
void AddCommand(Command command);
void RemoveCommand(const char* name);

// Styling
bool GetStyleFlag(ImCmdTextType type, ImCmdTextFlag flag);
void SetStyleFlag(ImCmdTextType type, ImCmdTextFlag flag, bool enabled);
ImFont* GetStyleFont(ImCmdTextType type);
void SetStyleFont(ImCmdTextType type, ImFont* font);
ImU32 GetStyleColor(ImCmdTextType type);
void SetStyleColor(ImCmdTextType type, ImU32 color);
void ClearStyleColor(ImCmdTextType type); //< Clear the style color for the given type, defaulting to ImGuiCol_Text

// Command palette widget
void SetNextCommandPaletteSearch(const char* text);
void SetNextCommandPaletteSearchBoxFocused();
void CommandPalette(const char* name);
bool IsAnyItemSelected();

void RemoveCache(const char* name);
void RemoveAllCaches();

// Command palette widget in a window helper
void SetNextWindowAffixedTop(ImGuiCond cond = 0);
void CommandPaletteWindow(const char* name, bool* p_open);

// Command responses, only call these in command callbacks (except TerminatingCallback)
void Prompt(std::vector<std::string> options);

} // namespace ImCmd
