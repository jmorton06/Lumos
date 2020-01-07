local api = premake.api
local action = premake.action

-- Add iOS to the allowed system values and `os` command line option
api.addAllowed("system", { "ios" })
api.addAllowed('language', 'ObjCpp' )
table.insert(premake.option.list["os"].allowed, { "ios", "Apple iOS" })

-- The `xcode4` action hardcodes OS to `macosx`. Unset that.
local act = action.get("xcode4")
if act then
	act.os = nil
end

filter "system:ios"
	--kind "StaticLib"
	xcodebuildsettings
	{
		["ARCHS"] = "$(ARCHS_STANDARD)",
		["ONLY_ACTIVE_ARCH"] = "NO",
		["SDKROOT"] = "iphoneos",
		["SUPPORTED_PLATFORMS"] = "iphonesimulator iphoneos",
		["CODE_SIGN_IDENTITY[sdk=iphoneos*]"] = "iPhone Developer",
		['IPHONEOS_DEPLOYMENT_TARGET'] = '12.1',
		['INFOPLIST_FILE'] = "../Lumos/src/Platform/iOS/Info.plist",
	}