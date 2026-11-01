newoption 
{
   trigger     = "renderer",
   value       = "API",
   description = "Choose a renderer",
   allowed = 
   {
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
		{"arm64", "arm64"},
		{"universal", "macOS: Apple Silicon + Intel"}
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

newoption
{
	trigger     = "game-project",
	value       = "path",
	description = "Path to game project directory to bundle into standalone app"
}

newoption
{
	trigger     = "starter-project",
	value       = "path",
	description = "Project bundled into the iOS editor as its first-run project (keeps the editor's own name and bundle id)"
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