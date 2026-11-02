-- Reads --game-project option and populates game_project global table
-- Used by Runtime/Editor premake5.lua to bundle a custom project into a standalone app

game_project = nil

function LoadGameProject()
	local project_path = _OPTIONS["game-project"]
	if not project_path then
		return
	end

	-- Normalize path
	if project_path:sub(-1) == "/" or project_path:sub(-1) == "\\" then
		project_path = project_path:sub(1, -2)
	end

	-- Make absolute if relative
	if not path.isabsolute(project_path) then
		project_path = path.join(os.getcwd(), project_path)
	end

	-- Extract folder name
	local folder_name = path.getname(project_path)

	-- Find .lmproj file
	local lmproj_path = nil
	local matches = os.matchfiles(project_path .. "/*.lmproj")
	if #matches > 0 then
		lmproj_path = matches[1]
	end

	if not lmproj_path then
		print("ERROR: No .lmproj found in " .. project_path)
		return
	end

	-- Read and parse JSON
	local f = io.open(lmproj_path, "r")
	if not f then
		print("ERROR: Cannot open " .. lmproj_path)
		return
	end
	local content = f:read("*a")
	f:close()

	-- Extract fields from JSON via pattern matching (avoiding dependency on json lib)
	local function json_string(key)
		local pattern = '"' .. key .. '"%s*:%s*"([^"]*)"'
		return content:match(pattern)
	end

	local function json_number(key)
		local pattern = '"' .. key .. '"%s*:%s*(%-?%d+)'
		local v = content:match(pattern)
		return v and tonumber(v) or nil
	end

	local function json_bool(key)
		local pattern = '"' .. key .. '"%s*:%s*(%a+)'
		local v = content:match(pattern)
		if v == "true" then return true end
		if v == "false" then return false end
		return nil
	end

	local title = json_string("Title") or folder_name
	local bundle_id = json_string("BundleIdentifier")
	local version = json_string("Version") or "1.0.0"
	local build_number = json_string("BuildNumber") or "1"
	local icon_path = json_string("IconPath")

	-- Mobile / distribution (Version 13)
	local orientation     = json_number("Orientation") or 0   -- 0=all, 1=portrait, 2=landscape
	local device_family   = json_number("DeviceFamily") or 3  -- 1=iPhone, 2=iPad, 3=both
	local min_ios         = json_string("MinIOSVersion") or "16.0"
	local status_bar_hidden = json_bool("StatusBarHidden")
	if status_bar_hidden == nil then status_bar_hidden = true end
	local non_exempt_enc  = json_bool("UsesNonExemptEncryption") or false
	local camera_usage    = json_string("CameraUsage") or ""
	local mic_usage       = json_string("MicrophoneUsage") or ""
	local photo_usage     = json_string("PhotoLibraryUsage") or ""
	local location_usage  = json_string("LocationUsage") or ""

	-- Auto-generate bundle_id from title if not specified
	if not bundle_id or bundle_id == "" then
		local safe_name = title:lower():gsub("[^%a%d]", "")
		bundle_id = "com.lumos." .. safe_name
	end

	-- Project name for premake (no spaces, filesystem-safe)
	local project_name = title:gsub("[^%a%d_]", "")
	if project_name == "" then
		project_name = folder_name:gsub("[^%a%d_]", "")
	end

	-- Resolve VFS icon path to absolute path
	local icon_abs_path = nil
	if icon_path and icon_path ~= "" then
		-- Strip VFS prefix (//)
		local resolved = icon_path:gsub("^//", "")
		icon_abs_path = path.join(project_path, resolved)
	end

	game_project = {
		dir          = project_path,
		rel_dir      = path.getrelative(os.getcwd(), project_path),
		name         = project_name,
		folder_name  = folder_name,
		title        = title,
		bundle_id    = bundle_id,
		version      = version,
		build_number = build_number,
		icon_path    = icon_path,
		icon_abs_path = icon_abs_path,
		lmproj       = lmproj_path,
		orientation       = orientation,
		device_family     = device_family,
		min_ios           = min_ios,
		status_bar_hidden = status_bar_hidden,
		non_exempt_enc    = non_exempt_enc,
		camera_usage      = camera_usage,
		mic_usage         = mic_usage,
		photo_usage       = photo_usage,
		location_usage    = location_usage
	}

	print("Game project: " .. game_project.title .. " (" .. game_project.dir .. ")")
	print("  Bundle ID:  " .. game_project.bundle_id)
	print("  Version:    " .. game_project.version .. " (" .. game_project.build_number .. ")")
end
