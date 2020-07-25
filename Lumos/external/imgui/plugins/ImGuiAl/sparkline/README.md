# ImGuiAl::Sparkline

A wrapper around `ImGui::PlotLines` that maintains the history of values. The length of the history is fixed at compile time.

## Usage

Include `imguial_sparkline.h`, and declare a `ImGuiAl::Sparkline` variable somewhere. Set the lower and upper limits of the plot area with `setLimits`, and add new values to the graph with `addValue`. Draw the widget with `draw`.

```C++
ImGuiAl::Sparkline<uint8_t, 256> _line;
_line.setLimits(0, 255);

...

_line.addValue(value);

ImVec2 max = ImGui::GetContentRegionAvail();
_line.draw(max);
```

Call `clear` to zero all values in the history.
