# Building & Deploying Lumos

Lumos uses **Premake5** to generate platform-native build projects. Prebuilt premake binaries are in `Tools/`.

## Start here: which steps do I need?

Three things get built, and they depend on each other in one direction:

| Piece | What it is | Built by | Needed for |
|---|---|---|---|
| **Editor** (`LumosEditor.app`) | the editor, and also the `lumos` command-line tool (`Scripts/lumos` just runs the newest editor build) | `./Scripts/MacOS/BuildMacOS-Editor.sh` | editing, and every `lumos` command below |
| **Runtime** (`Runtime.app`) | the engine running one project, used for scripted tests (`--test=`) on the Mac | `./Scripts/MacOS/BuildMacOS.sh` | `Tests/run.sh`, quick runs on the Mac |
| **Export templates** | prebuilt engine libraries per platform that exports link against (`Templates/<version>/<platform>/`) | `Scripts/lumos templates build --platform=ios` (or `macos`) | `lumos export` / `lumos deploy` |

Exports never compile the engine: they take a template and add your project. So **a template is only as new as the last time you built it.**

### 1. I only changed Lua / assets in a game

```bash
Scripts/lumos deploy --project=../Orion --preset=iOS     # phone (~30 s)
Scripts/lumos export --project=../Orion --preset=macOS --run
```

No engine build needed, as long as the editor and template already exist.

### 2. I changed engine C++ and want to try it on the Mac

```bash
./Scripts/MacOS/BuildMacOS-Editor.sh     # editor (+ lumos CLI)
./Scripts/MacOS/BuildMacOS.sh            # Runtime, for tests
```

### 3. I changed engine C++ and want it on the phone

Do these **in this order**. Each step needs the one before it:

```bash
./Scripts/MacOS/BuildMacOS-Editor.sh              # 1. lumos is the editor - build it first
Scripts/lumos templates build --platform=ios      # 2. engine for iOS -> Templates/<version>/ios/
Scripts/lumos deploy --project=../Orion --preset=iOS   # 3. game + new template -> phone
```

Add `./Scripts/MacOS/BuildMacOS.sh` if you also want to run the Mac tests.

### Things that catch people out

- **Changed a header (`.h`)? Clean first.** The generated Xcode projects don't track header dependencies. Only the `.cpp` files you edited get recompiled, and the rest keep the old object layout, which usually shows up as a crash at startup. Run `./Scripts/MacOS/CleanMacOS.sh` (and `rm -rf bin-int/lumos-ios-template-*` for the iOS template) before building.
- **The Xcode projects are shared between macOS and iOS.** Building an iOS template (or `BuildIOS.sh`) regenerates them for iOS. The template build now puts the macOS ones back when it finishes, and the macOS build scripts regenerate them if they find iOS ones. If you build from inside Xcode instead, run `./Scripts/MacOS/GenerateXCode.sh` first.
- **New `.cpp` file?** Regenerate the projects (`./Scripts/MacOS/GenerateXCode.sh`) before building.
- **Building the template from the editor** (Export window > *Build Template*) does the same as `lumos templates build`. Earlier editors could crash there: a compiler command line in the log overflowed the text renderer. That is fixed; the command line is still the easier way to watch a long build.
- **The phone has to be unlocked** for `deploy` to launch the app (it installs either way).
- **Another session or person running `Tests/run.sh` in the same project** at the same time can feed you stale scripts: it moves `Assets.lpak` aside and back.

## Quick Reference

| What | Command |
|------|---------|
| Build Editor (macOS) | `./Scripts/MacOS/BuildMacOS-Editor.sh` |
| Build Runtime (macOS) | `./Scripts/MacOS/BuildMacOS.sh` |
| Build iOS export template | `Scripts/lumos templates build --platform=ios` |
| Build macOS export template | `Scripts/lumos templates build --platform=macos` |
| Deploy a game to an iPhone | `Scripts/lumos deploy --project=<dir> --preset=iOS` |
| Export a macOS app | `Scripts/lumos export --project=<dir> --preset=macOS` |
| Connected iOS devices | `Scripts/lumos devices` |
| Build Runtime (Linux) | `./Scripts/Linux/BuildLinux.sh` |
| Build Editor (Linux) | `./Scripts/Linux/BuildLinux-Editor.sh` |
| Build Runtime (Windows) | `Scripts\Windows\BuildWindows.bat` |
| Build Editor (Windows) | `Scripts\Windows\BuildWindows-Editor.bat` |

Exporting, presets, signing and the App Store are covered in [Packaging.md](Packaging.md).

---

## Configurations

All platforms support three build configurations:

| Config | Use | Symbols | Optimize | Tracy |
|--------|-----|---------|----------|-------|
| **Debug** | Development | On | Off | On |
| **Release** | Testing | On | Speed | On |
| **Production** | Distribution | Off | Full | Off |

---

## macOS

### Generate Xcode Projects

```bash
./Scripts/MacOS/GenerateXCode.sh
```

This runs `Tools/premake5 xcode4` and patches the generated `.pbxproj` files to fix `SKIP_INSTALL` values for proper archiving.

Output:
- `Runtime/Runtime.xcodeproj`
- `Editor/LumosEditor.xcodeproj`

### Build Runtime

```bash
./Scripts/MacOS/BuildMacOS.sh
```

Builds Release config via xcodebuild. Output: `bin/Release-macosx-x86_64/Runtime.app`

### Build Editor

```bash
./Scripts/MacOS/BuildMacOS-Editor.sh
```

Output: `bin/Release-macosx-x86_64/LumosEditor.app`

### Run

```bash
./Scripts/MacOS/RunMacOS.sh          # Runtime
./Scripts/MacOS/RunMacOS-Editor.sh   # Editor
```

### Clean

```bash
./Scripts/MacOS/CleanMacOS.sh
```

Runs `xcodebuild clean` on both Runtime and Editor projects.

---

## iOS

For a **game on your phone**, use `Scripts/lumos deploy` (see *Start here* above and [Packaging.md](Packaging.md)). It builds nothing from source and doesn't touch the Xcode projects.

The scripts below build the engine's own iOS app from source (engine work, the iOS editor). Both regenerate the shared Xcode projects **for iOS**, so run `./Scripts/MacOS/GenerateXCode.sh` before building for macOS in Xcode again (the macOS build scripts do it for you).

```bash
./Scripts/MacOS/GenerateXCode-iOS.sh     # premake --os=ios; then open the project in Xcode
./Scripts/MacOS/BuildIOS.sh              # same, then builds with xcodebuild (no signing)
./Scripts/MacOS/DeployEditor-iOS.sh      # the editor itself on a phone
```

---

## Linux

### Generate Makefiles

```bash
Tools/premake5 gmake2
```

### Build

```bash
./Scripts/Linux/BuildLinux.sh          # Runtime
./Scripts/Linux/BuildLinux-Editor.sh   # Editor
```

These run `make Runtime` / `make LumosEditor` from the generated gmake2 project.

### Run

```bash
./Scripts/Linux/RunLinux.sh            # Runtime
./Scripts/Linux/RunLinux-Editor.sh     # Editor
```

---

## Windows

### Generate Visual Studio Solution

```bash
Scripts\Windows\GenerateVS22.bat    # Visual Studio 2022
Scripts\Windows\GenerateVS19.bat    # Visual Studio 2019
Scripts\Windows\GenerateVS17.bat    # Visual Studio 2017
```

These call `Tools\premake5.exe vs20XX` to generate `.sln` and `.vcxproj` files.

### Build

```bash
Scripts\Windows\BuildWindows.bat         # Runtime (Release, x64)
Scripts\Windows\BuildWindows-Editor.bat  # Editor (Release, x64)
```

Uses `vswhere` to find MSBuild and builds with `/p:Platform=x64 /p:Configuration=Release`.

---

## Premake Options

All options are passed to `Tools/premake5`:

```bash
Tools/premake5 [action] [options]
```

### Actions

| Action | Platform | Output |
|--------|----------|--------|
| `xcode4` | macOS/iOS | `.xcodeproj` |
| `gmake2` | Linux | Makefiles |
| `vs2022` | Windows | `.sln` + `.vcxproj` |
| `clean` | All | Deletes `build/`, `bin/`, `bin-int/` |

### Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `--arch` | `x86`, `x64`, `arm`, `arm64` | x64 (desktop), arm (mobile) | Target architecture |
| `--os` | `ios` | host OS | Target iOS instead of macOS |
| `--teamid` | Apple Team ID | from settings | Override Apple Developer Team |
| `--tvOS` | target | - | Target tvOS |
| `--game-project` | path | - | Bundle a game project into standalone app |

### Examples

```bash
# iOS project with specific team
Tools/premake5 xcode4 --os=ios --teamid=C5L4T5BF6L

# Linux makefiles for ARM
Tools/premake5 gmake2 --arch=arm64

# Bundle a custom game (see Packaging section below)
Tools/premake5 xcode4 --game-project=Tools/LumosWall
```

---

## Packaging Standalone Games (legacy)

Use the export system instead: `Scripts/lumos export` / `deploy` / `archive`, described in [Packaging.md](Packaging.md). It uses prebuilt templates, so it doesn't recompile the engine for every game.

`./Scripts/MacOS/PackageGame.sh <project-dir> [--ios]` is the old route. It regenerates the shared `Runtime` Xcode project around one game and builds it, or for `--ios` only generates the project for you to sign and build in Xcode. It still works, but it leaves the shared projects pointing at that game.

---

## Profiling

### Build Tracy Tools

```bash
./Scripts/MacOS/BuildTracyCapture.sh
./Scripts/MacOS/BuildTracyCsvExport.sh
```

Outputs `Tools/Tracy/tracy-capture` and `Tools/Tracy/tracy-csvexport`.

### Capture a Profile

```bash
./Scripts/profile-capture.sh [frames] [scene] [output_name]

# Examples
./Scripts/profile-capture.sh                    # 300 frames, Sponza
./Scripts/profile-capture.sh 500 Sandbox perf   # 500 frames, Sandbox scene
```

Output: `Profiles/<name>.tracy` - open with Tracy profiler GUI.

Tracy is enabled in Debug and Release configs (`TRACY_ENABLE`), disabled in Production.

---

## Runtime CLI Options

```bash
# Run with a specific project
./Runtime --project=Tools/LumosWall

# Benchmark mode (quit after N frames)
./Runtime --benchmark=100
```

---

## Output Directory Structure

```
bin/
  Debug-macosx-x86_64/
    Runtime.app/
    LumosEditor.app/
  Release-macosx-x86_64/
    Runtime.app/
    LumosEditor.app/
  Production-macosx-x86_64/
    MyGame.app/              # From PackageGame.sh
bin-int/
  <config>-<system>-<arch>/obj/  # Intermediate files
```

Format: `<Configuration>-<System>-<Architecture>`
