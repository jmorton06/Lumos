# ImGuiAl::Button

A button that can be disabled.

## Usage

Include `imguial_button.h` and use `ImGuiAl::Button` instead of `ImGui::Button`:

```C++
bool Button( const char* label, bool enabled = true, const ImVec2& size = ImVec2( 0, 0 ) )
```

When the `enabled` parameter is `true`, this button behaves exactly like the regular imgui button. If `enabled` is `false`, the button is rendered with black text on a dark dray background and doesn't respond to mouse clicks.