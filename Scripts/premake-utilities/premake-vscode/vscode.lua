--
-- Name:        vscode.lua
-- Purpose:     Define the vscode action(s).
-- Author:      Ryan Pusztai
-- Modified by: Andrea Zanellato
--              Andrew Gough
--              Manu Evans
--              Jason Perkins
--              Yehonatan Ballas
-- Created:     2013/05/06
-- Copyright:   (c) 2008-2020 Jason Perkins and the Premake project
--

local p = premake

p.modules.vscode = {}
p.modules.vscode._VERSION = p._VERSION

local vscode = p.modules.vscode
local project = p.project


function vscode.generateWorkspace(wks)
    p.eol("\r\n")
    p.indent("  ")
    
    p.generate(wks, ".code-workspace", vscode.workspace.generate)
end

function vscode.generateProject(prj)
    p.eol("\r\n")
    p.indent("  ")

    if project.isc(prj) or project.iscpp(prj) then
        p.generate(prj, prj.location .. '/' .. prj.name .. "/.vscode/tasks.json", vscode.project.vscode_tasks)
        p.generate(prj, prj.location .. '/' .. prj.name .. "/.vscode/launch.json", vscode.project.vscode_launch)
        p.generate(prj, prj.location .. '/' .. prj.name .. "/.vscode/c_cpp_properties.json", vscode.project.vscode_c_cpp_properties)
    end
end

function vscode.cleanWorkspace(wks)
    p.clean.file(wks, wks.name .. ".code-workspace")
end

function vscode.cleanProject(prj)
    p.clean.file(prj, prj.name .. ".vscode")
end

include("vscode_workspace.lua")
include("vscode_project.lua")

include("_preload.lua")

return vscode
