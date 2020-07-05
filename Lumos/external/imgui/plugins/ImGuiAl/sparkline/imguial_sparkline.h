/*
The MIT License (MIT)

Copyright (c) 2019 Andre Leiradella

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <imgui.h>
#include <stddef.h>
#include <stdio.h>

namespace ImGuiAl {
  template<typename T, size_t L>
  class Sparkline {
    public:
      Sparkline() {
        setLimits(0, 1);
        clear();
      }

      void setLimits(T min, T max) {
        _min = static_cast<float>(min);
        _max = static_cast<float>(max);
      }

      void addValue(T value) {
        memmove((void*)_values, (void*)(_values + 1), (L - 1) * sizeof(T));
        _values[L - 1] = value;
      }

      void clear() {
        memset((void*)_values, 0, L * sizeof(T));
      }

      void draw() {
        ImVec2 max = ImGui::GetContentRegionAvail();
        draw(max);
      }

      void draw(const ImVec2& size) {
        char overlay[32];
        format(overlay, sizeof(overlay), _values[L - 1]);

        ImGui::PushID(static_cast<void*>(this));
        ImGui::PlotLines("##plot", getValue, static_cast<void*>(this), L, 0, overlay, _min, _max, size);
        ImGui::PopID();
      }
      
    protected:
      float _min, _max;
      T _values[L];

      static float getValue(void* data, int idx) {
        Sparkline* self = static_cast<Sparkline*>(data);
        return static_cast<float>(self->_values[idx]);
      }

      static void format(char* buffer, size_t bufferLen, int value) {
        snprintf(buffer, bufferLen, "%d", value);
      }

      static void format(char* buffer, size_t bufferLen, double value) {
        snprintf(buffer, bufferLen, "%f", value);
      }
  };
}
