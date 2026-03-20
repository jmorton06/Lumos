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

	local title = json_string("Title") or folder_name
	local bundle_id = json_string("BundleIdentifier")
	local version = json_string("Version") or "1.0.0"
	local build_number = json_string("BuildNumber") or "1"
	local icon_path = json_string("IconPath")

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
		lmproj       = lmproj_path
	}

	print("Game project: " .. game_project.title .. " (" .. game_project.dir .. ")")
	print("  Bundle ID:  " .. game_project.bundle_id)
	print("  Version:    " .. game_project.version .. " (" .. game_project.build_number .. ")")
end
