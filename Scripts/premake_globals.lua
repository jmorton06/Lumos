-- setup global variables to configure the projects
platform_dir = ""
platform = ""
build_cmd = ""
link_cmd = ""
renderer_dir = ""
sdk_version = ""
shared_libs_dir = ""

-- setup functions
function setup_from_options()
    if _OPTIONS["renderer"] then
        renderer_dir = _OPTIONS["renderer"]
    end

    if _OPTIONS["sdk_version"] then
        sdk_version = _OPTIONS["sdk_version"]
    end

    if _OPTIONS["platform_dir"] then
        platform_dir = _OPTIONS["platform_dir"]
    end

    if _OPTIONS["toolset"] then
        toolset(_OPTIONS["toolset"])
    end
end

function script_path()
   local str = debug.getinfo(2, "S").source:sub(2)
   return str:match("(.*/)")
end

function windows_sdk_version()
	if sdk_version ~= "" then
		return sdk_version
	end
	return "latest"
end

function setup_from_action()
    if _ACTION == "gmake" then
        if platform_dir == "linux" then
            build_cmd = "-std=c++17"
        else
            build_cmd = "-std=c++17 -stdlib=libc++"
            link_cmd = "-stdlib=libc++"
        end
    elseif _ACTION == "xcode4" then 
        platform_dir = "osx" 
        if not renderer_dir then
            renderer_dir = "opengl"
        end
        if _OPTIONS["xcode_target"] then
            platform_dir = _OPTIONS["xcode_target"]
        end
        if platform_dir == "ios" then
            build_cmd = "-std=c++17 -stdlib=libc++"
            link_cmd = "-stdlib=libc++"
        else if platform_dir == "tvOS"
            build_cmd = "-std=c++17 -stdlib=libc++"
            link_cmd = "-stdlib=libc++"
        else
            build_cmd = "-std=c++17 -stdlib=libc++"
            link_cmd = "-stdlib=libc++ -mmacosx-version-min=10.8"
        end
    elseif _ACTION == "android-studio" then 
        build_cmd = { "-std=c++17" }
    elseif _ACTION == "vs2017" then
        platform_dir = "win32" 
        build_cmd = "/Ob1" -- use force inline
    end
    
    platform = platform_dir
	
	print("platform: " .. platform)
	print("renderer: " .. renderer_dir)
	print("sdk version: " .. windows_sdk_version())
    
end

-- setup product
function setup_product_ios(name)
	xcodebuildsettings {
		["PRODUCT_BUNDLE_IDENTIFIER"] = name
	}
end

function setup_product(name)
    if platform == "ios" then setup_product_ios(name)
    end

    if platform == "tvOS" then setup_product_ios(name)
    end
end

-- setup env - inserts architecture, platform and sdk settings
function setup_env_ios()
	xcodebuildsettings {
		["ARCHS"] = "$(ARCHS_STANDARD)",
		["SDKROOT"] = "iphoneos"
	}
	if _OPTIONS["teamid"] then
		xcodebuildsettings {
			["DEVELOPMENT_TEAM"] = _OPTIONS["teamid"]
		}
	end
end

function setup_env_tvos()
	xcodebuildsettings {
		["ARCHS"] = "$(ARCHS_STANDARD)",
		["SDKROOT"] = "tvos"
	}
	if _OPTIONS["teamid"] then
		xcodebuildsettings {
			["DEVELOPMENT_TEAM"] = _OPTIONS["teamid"]
		}
	end
end

function setup_env_osx()
	xcodebuildsettings {
		["MACOSX_DEPLOYMENT_TARGET"] = "10.14"
	}
	architecture "x64"
end

function setup_env_win32()
	architecture "x64"
end

function setup_env_linux()
	architecture "x64"
end

function setup_env()
    if platform == "ios" then setup_env_ios()
    elseif platform == "osx" then setup_env_osx()
    elseif platform == "tvOS" then setup_env_tvos()
    elseif platform == "win32" then setup_env_win32()
    elseif platform == "linux" then setup_env_linux()
    end
end

-- setup platform defines - inserts platform defines for porting macros
function setup_platform_defines()
	defines
	{
		("LUMOS_PLATFORM_" .. string.upper(platform)),
	}
end

-- entry
setup_from_options()
setup_from_action()