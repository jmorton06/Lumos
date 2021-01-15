newoption 
{
   trigger     = "renderer",
   value       = "API",
   description = "Choose a renderer",
   allowed = 
   {
      { "opengl", "OpenGL (macOS, linux, Windows)" },
      --{ "dx11",  "DirectX 11 (Windows only)" },
      --{ "metal", "Metal (macOS, iOS only)" },
      { "vulkan", "Vulkan (Windows, linux, iOS, macOS)" }
   }
}

newoption
{
	trigger = "arch",
	value   = "arch",
	description = "Choose architecture",
	allowed = 
	{
		{"x86", "x86" },
		{"x64", "x64" },
		{"arm", "arm" },
		{"arm64", "arm64"}
	}
}

newoption
{
	trigger     = "teamid",
	value	    = "id",
	description = "development team id for apple developers"
}

newoption
{
	trigger     = "tvOS",
	value	    = "target",
	description = "Target tvOS"
}

newaction
{
	trigger     = "clean",
	description = "clean the software",
	execute     = function ()
		print("clean the build...")
		os.rmdir("./build")
		os.rmdir("./bin")
		os.rmdir("./bin-int")
		print("done.")
	end
}