# ImGuiAl::Fonts

This is not really a widget, but some fonts conveniently packed for use with [imgui](https://github.com/ocornut/imgui):

* [Cousine Regular](https://www.google.com/fonts/specimen/Cousine) and [Droid Sans](https://www.fontsquirrel.com/fonts/droid-sans) (Google has removed Droid Sans from their website it seems)
* [Karla Regular](https://www.google.com/fonts/specimen/Karla)
* Many Tristan Grimmer [programming fonts](http://upperbounds.net/)
* [Font Awesome](http://fontawesome.io/) versions 4 and 5 (does *not* include the pro fonts)
* [Fork Awesome](https://forkaweso.me/Fork-Awesome/)
* [Material Design Icons](https://materialdesignicons.com/)
* Google's [Material Design Icons](https://design.google.com/icons/)
* [Kenney Icon Font](https://github.com/SamBrishes/kenney-icon-font)
* [Ionicons](https://ionicons.com/)

The fonts are presented compressed and as C arrays, ready to be embedded:

```cpp
#include "IconsMaterialDesign.h"
#include "MaterialDesign.inl"

static const ImWchar ranges[] = {ICON_MIN_MD, ICON_MAX_MD, 0};
ImFontConfig config;
config.MergeMode = true;
ImGuiIO& io = ImGui::GetIO();
io.Fonts->AddFontDefault();
io.Fonts->AddFontFromMemoryCompressedTTF(MaterialDesign_compressed_data, MaterialDesign_compressed_size, 24.0f, &config, ranges);
```

Also use Juliette Foucaut's [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders) to easily add icons to your captions:

```cpp
bool loadGamePressed = ImGui::Button(ICON_FA_ROCKET " Load game...");
```
