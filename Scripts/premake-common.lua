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
   trigger     = "xcode_target",
   value       = "TARGET",
   description = "Choose an xcode build target",
   allowed = 
   {
      { "osx", "OSX" },
      { "ios",  "iOS" },
      { "tvOS", "TVOS" },
   }
}

newoption
{
	trigger     = "teamid",
	value	    = "id",
	description = "development team id for apple developers"
}
