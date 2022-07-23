project "imgui"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "On"

	files
	{
		"imgui/imconfig.h",
		"imgui/imgui.h",
		"imgui/imgui.cpp",
		"imgui/imgui_draw.cpp",
		"imgui/imgui_internal.h",
		"imgui/imgui_widgets.cpp",
		"imgui/imgui_tables.cpp",
		"imgui/imstb_rectpack.h",
		"imgui/imstb_textedit.h",
		"imgui/imstb_truetype.h",
		"imgui/imgui_demo.cpp",
		"imgui/Plugins/ImGuizmo.h",
		"imgui/Plugins/ImGuizmo.cpp",
		--"imgui/Plugins/ImCurveEdit.h",
		--"imgui/Plugins/ImCurveEdit.cpp",
		"imgui/Plugins/ImTextEditor.h",
		"imgui/Plugins/ImTextEditor.cpp",
	}

	includedirs
	{
		"imgui/",
		"../"
	}

	filter "system:windows"
		systemversion "latest"

	filter "system:linux"
		pic "On"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	filter "configurations:Production"
		runtime "Release"
		optimize "on"