#pragma once

namespace Lumos::Export
{
    // `LumosEditor <command> ...` (export, deploy, run, pack, icons, devices, presets, templates, help).
    // Returns false when argv[1] isn't a command, so the editor starts as usual.
    bool RunCommandLine(int argc, char** argv, int& exitCode);
    // argv[1] is one of those commands (checked before the engine starts, to quieten its logging)
    bool IsCommand(int argc, char** argv);
}
