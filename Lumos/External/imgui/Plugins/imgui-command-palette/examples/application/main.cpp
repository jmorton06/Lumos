#include "imcmd_command_palette.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glad/glad.h>
#include <imgui.h>
#include <iostream>
#include <string>

#include <../res/bindings/imgui_impl_glfw.h>
#include <../res/bindings/imgui_impl_opengl3.h>

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include <../res/bindings/imgui_impl_glfw.cpp>
#include <../res/bindings/imgui_impl_opengl3.cpp>

static void GlfwErrorCallback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main()
{
    if (!glfwInit()) {
        return -1;
    }

    glfwSetErrorCallback(&GlfwErrorCallback);

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui Command Palette Example", nullptr, nullptr);
    if (window == nullptr) {
        return -2;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return -3;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Note: if you only have one context, simply call ImCmd::CreateContext(); here
    constexpr int kContextCount = 4;
    ImCmd::Context* contexts[kContextCount];
    int current_context = 0;
    for (int i = 0; i < kContextCount; ++i) {
        contexts[i] = ImCmd::CreateContext();
    }

    auto& io = ImGui::GetIO();
    auto regular_font = io.Fonts->AddFontFromFileTTF("fonts/NotoSans-Regular.ttf", 16, nullptr, io.Fonts->GetGlyphRangesDefault());
    auto bold_font = io.Fonts->AddFontFromFileTTF("fonts/NotoSans-Bold.ttf", 16, nullptr, io.Fonts->GetGlyphRangesDefault());

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImCmd::SetStyleFont(ImCmdTextType_Regular, regular_font);
    ImCmd::SetStyleFont(ImCmdTextType_Highlight, bold_font);

    bool show_demo_window = true;
    bool show_command_palette = false;
    bool show_custom_command_palette = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImCmd::Command toggle_demo_cmd;
    toggle_demo_cmd.Name = "Toggle ImGui demo window";
    toggle_demo_cmd.InitialCallback = [&]() {
        show_demo_window = !show_demo_window;
    };
    ImCmd::AddCommand(std::move(toggle_demo_cmd));

    ImCmd::Command select_theme_cmd;
    select_theme_cmd.Name = "Select theme";
    select_theme_cmd.InitialCallback = [&]() {
        ImCmd::Prompt(std::vector<std::string>{
            "Classic",
            "Dark",
            "Light",
        });
    };
    select_theme_cmd.SubsequentCallback = [&](int selected_option) {
        switch (selected_option) {
            case 0: ImGui::StyleColorsClassic(); break;
            case 1: ImGui::StyleColorsDark(); break;
            case 2: ImGui::StyleColorsLight(); break;
            default: break;
        }
    };
    ImCmd::AddCommand(std::move(select_theme_cmd));

    ImCmd::Command example_cmd;
    example_cmd.Name = "Example command";

    ImCmd::Command add_example_cmd_cmd;
    add_example_cmd_cmd.Name = "Add 'Example command'";
    add_example_cmd_cmd.InitialCallback = [&]() {
        ImCmd::AddCommand(example_cmd);
    };

    ImCmd::Command remove_example_cmd_cmd;
    remove_example_cmd_cmd.Name = "Remove 'Example command'";
    remove_example_cmd_cmd.InitialCallback = [&]() {
        ImCmd::RemoveCommand(example_cmd.Name.c_str());
    },

    ImCmd::AddCommand(example_cmd); // Copy intentionally
    ImCmd::AddCommand(std::move(add_example_cmd_cmd));
    ImCmd::AddCommand(std::move(remove_example_cmd_cmd));

    bool use_highlight_underline = false;
    bool use_highlight_font = true;
    bool use_highlight_font_color = false;
    ImVec4 highlight_font_color(1.0f, 0.0f, 0.0f, 1.0f);

    int custom_counter = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_P)) {
            show_command_palette = !show_command_palette;
        }
        if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_O)) {
            show_custom_command_palette = !show_custom_command_palette;
        }

        if (show_command_palette) {
            ImCmd::CommandPaletteWindow("CommandPalette", &show_command_palette);
        }
        if (show_custom_command_palette) {
            ImCmd::SetNextWindowAffixedTop(ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(ImGui::GetMainViewport()->Size.x * 0.3f, 0.0f), ImGuiCond_Once);
            ImGui::Begin("CustomCommandPalette");

            ImGui::Text("Hi! This is a custom label");
            ImGui::Text("You may also notice that this window has a title bar and is movable - that's also custom");

            ImGui::Separator();

            ImGui::Text("Counter: %d", custom_counter);
            ImGui::SameLine();
            if (ImGui::Button("Click me")) {
                ++custom_counter;
            }

            // BEGIN command palette widget
            // Note: see ImCmd::CommandPaletteWindow for all the default behaviors, we've omitted some here
            if (ImGui::IsWindowAppearing()) {
                ImCmd::SetNextCommandPaletteSearchBoxFocused();
            }

            ImCmd::CommandPalette("");

            if (ImCmd::IsAnyItemSelected()) {
                show_custom_command_palette = false;
            }
            // END command palette widget

            ImGui::End();
        }

        ImGui::Begin("Config", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        {
            char label[] = "Context X";
            //              ^       ^^
            // Index:       0       8\ null terminator
            label[8] = '0' + (current_context + 1);

            if (ImGui::BeginCombo("ImCmd context", label)) {
                for (int i = 0; i < kContextCount; ++i) {
                    label[8] = '0' + (i + 1);
                    if (ImGui::Selectable(label, current_context == i)) {
                        current_context = i;
                        ImCmd::SetCurrentContext(contexts[i]);
                    }
                }
                ImGui::EndCombo();
            }
        }
        ImGui::Text("Press Ctrl+Shift+P to bring up the default command palette - CommandPaletteWindow()");
        ImGui::Text("Press Ctrl+Shift+O to bring up a command palette placed inside a custom window - CommandPalette()");
        if (ImGui::Checkbox("Use underline for highlights", &use_highlight_underline)) {
            ImCmd::SetStyleFlag(ImCmdTextType_Highlight, ImCmdTextFlag_Underline, use_highlight_underline);
        }
        if (ImGui::Checkbox("Use bold font for highlights", &use_highlight_font)) {
            if (use_highlight_font) {
                ImCmd::SetStyleFont(ImCmdTextType_Highlight, bold_font);
            } else {
                ImCmd::SetStyleFont(ImCmdTextType_Highlight, regular_font);
            }
        }
        if (ImGui::Checkbox("Use color for highlights", &use_highlight_font_color)) {
            if (use_highlight_font_color) {
                ImCmd::SetStyleColor(ImCmdTextType_Highlight, ImGui::ColorConvertFloat4ToU32(highlight_font_color));
            } else {
                ImCmd::ClearStyleColor(ImCmdTextType_Highlight);
            }
        }
        if (ImGui::ColorEdit3("Highlight color", &highlight_font_color.x)) {
            if (use_highlight_font_color) {
                ImCmd::SetStyleColor(ImCmdTextType_Highlight, ImGui::ColorConvertFloat4ToU32(highlight_font_color));
            }
        }
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    // Note: if you only have one context, simply call ImCmd::DestroyContext(); here
    for (int i = 0; i < kContextCount; ++i) {
        ImCmd::DestroyContext(contexts[i]);
        contexts[i] = 0;
    }
    // Technically not necessary, kept here for "correctness"
    ImCmd::SetCurrentContext(nullptr);

    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
