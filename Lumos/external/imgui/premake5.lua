project "imgui"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "On"

	files
	{
		"imconfig.h",
		"imgui.h",
		"imgui.cpp",
		"imgui_draw.cpp",
		"imgui_internal.h",
		"imgui_widgets.cpp",
		"imstb_rectpack.h",
		"imstb_textedit.h",
		"imstb_truetype.h",
		"imgui_demo.cpp",
		"plugins/ImGuizmo.h",
		"plugins/ImGuizmo.cpp",
		--"plugins/ImCurveEdit.h",
		--"plugins/ImCurveEdit.cpp",
		"plugins/ImTextEditor.h",
		"plugins/ImTextEditor.cpp",
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