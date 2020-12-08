--
-- Name:        vscode_project.lua
-- Purpose:     Generate a vscode C/C++ project file.
-- Author:      Ryan Pusztai
-- Modified by: Andrea Zanellato
--              Manu Evans
--              Tom van Dijck
--              Yehonatan Ballas
-- Created:     2013/05/06
-- Copyright:   (c) 2008-2020 Yehonatan Ballas, Jason Perkins and the Premake project
--

local p = premake
local tree = p.tree
local project = p.project
local config = p.config
local vscode = p.modules.vscode

vscode.project = {}
local m = vscode.project

local cpp_standard = {}
cpp_standard["C++98"] = 98
cpp_standard["C++11"] = 11
cpp_standard["C++14"] = 14
cpp_standard["C++17"] = 17
cpp_standard["C++20"] = 20
cpp_standard["gnu++98"] = 98
cpp_standard["gnu++11"] = 11
cpp_standard["gnu++14"] = 14
cpp_standard["gnu++17"] = 17
cpp_standard["gnu++20"] = 20

local build_task_name = "C/C++: build"

function m.getcompiler(cfg)
	local toolset = p.tools[_OPTIONS.cc or cfg.toolset or p.CLANG]
	if not toolset then
		error("Invalid toolset '" + (_OPTIONS.cc or cfg.toolset) + "'")
	end
	return toolset
end

-- cross platform symbolic link creation
function symlink(target, link)
	if os.host() == 'windows' then
		os.execute('cmd.exe /c mklink /d ' .. link .. ' ' .. target)
	else
		os.execute('ln -s -f ' .. target .. ' ' .. link)
	end
end

-- VS Code only scans for project files inside the project's directory, so symlink them into
-- the project's directory.
function m.files(prj)
	local node_path = ''
	local tr = project.getsourcetree(prj)
	tree.traverse(tr, {
		onbranchenter = function(node, depth)
			node_path = node_path .. '/' .. node.name
		end,
		onbranchexit = function(node, depth)
			node_path = node_path:sub(1, node_path:len()-(node.name:len()+1))
		end,
		onleaf = function(node, depth)
			local full_path = prj.location .. node_path
			os.mkdir(full_path)
			symlink(node.abspath, full_path)
		end
	}, true)
end


--
-- Project: Generate vscode tasks.json.
--
function m.vscode_tasks(prj)

	m.files(prj)

	p.utf8()
	--TODO task per project
	_p('{')
		_p(1, '"version": "2.0.0",')
		_p(1, '"tasks": [{')
			_p(2, '"type": "shell",')
			_p(2, '"label": "%s",', build_task_name)
			_p(2, '"command": "clear && time make -r -j`nproc`",')
			_p(2, '"args": [],')
			_p(2, '"options": {')
				_p(3, '"cwd": "${workspaceFolder}/../"')
			_p(2, '},')
			_p(2, '"problemMatcher": [')
				_p(3, '"$gcc"')
			_p(2, '],')
			_p(2, '"group": {')
				_p(3, '"kind": "build",')
				_p(3, '"isDefault": true')
			_p(2, '}')
		_p(1, '}]')
	_p('}')
end


--
-- Project: Generate vscode launch.json.
--
function m.vscode_launch(prj)
	p.utf8()
	_p('{')
		_p(1, '"version": "0.2.0",')
		_p(1, '"configurations": [')
		local first_cfg = true
		for cfg in project.eachconfig(prj) do
		if first_cfg then
			first_cfg = false
			_p(1, '{')
		else
			_p(1, ',{')
		end
			_p(2, '"name": "%s: Build and debug",', prj.name)
			_p(2, '"type": "cppdbg",')
			_p(2, '"request": "launch",')
			_p(2, '"program": "%s/%s",', cfg.buildtarget.directory, prj.name)
			_p(2, '"args": [],')
			_p(2, '"stopAtEntry": false,')
			_p(2, '"cwd": "${workspaceFolder}/../",')
			_p(2, '"environment": [],')
			_p(2, '"externalConsole": false,')
			_p(2, '"MIMode": "gdb",')
			_p(2, '"setupCommands": [{')
				_p(3, '"description": "Enable pretty-printing for gdb",')
				_p(3, '"text": "-enable-pretty-printing",')
				_p(3, '"ignoreFailures": true')
			_p(2, '}],')
			_p(2, '"preLaunchTask": "%s",', build_task_name)
			_p(2, '"miDebuggerPath": "/usr/bin/gdb"')
		_p(1, '}')
		end
		_p(1, ']')
	_p('}')
end


--
-- Project: Generate vscode c_cpp_properties.json.
--
function m.vscode_c_cpp_properties(prj)
	_p('{')
		_p(1, '"configurations": [')
		local first_cfg = true
		for cfg in project.eachconfig(prj) do
		if first_cfg then
			first_cfg = false
			_p(1, '{')
		else
			_p(1, ',{')
		end
			_p(2, '"name": "%s %s",', prj.name, cfg.name)
			_p(2, '"includePath": [')
				_p(3, '"${workspaceFolder}/**"')
				for _, includedir in ipairs(cfg.includedirs) do
					_p(3, ',"%s"', includedir)
				end
			_p(2, '],')
			_p(2, '"defines": [')
				if #cfg.defines > 0 then
					_p(3, '"%s"', cfg.defines[1])
					for i = 2,#cfg.defines do
						_p(3, ',"%s"', cfg.defines[i])
					end
				end
				
			_p(2, '],')
			_p(2, '"compilerPath": "/usr/bin/g++",') --TODO premake toolset
			if cfg.cdialect ~= nil then
				_p(2, '"cStandard": "%s",', cfg.cdialect:lower())
			end
			if cfg.cppdialect ~= nil then
				_p(2, '"cppStandard": "%s",', cfg.cppdialect:lower())
			end
			_p(2, '"intelliSenseMode": "gcc-x64",') --TODO premake toolset
			_p(2, '"compilerArgs": [')
				-- force includes
				local toolset = m.getcompiler(cfg)
				local forceincludes = toolset.getforceincludes(cfg)
				_p(3, '"' .. table.concat(forceincludes, ";") .. '"')
			_p(2, ']')
		_p(1, '}')
		end
		_p(1, '],')
		_p(1, '"version": 4')
	_p('}')
end