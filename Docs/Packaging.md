# Packaging & Export

Turn a Lumos project into an app you can hand to someone: a macOS `.app`, an iOS app on your phone (or for the App Store), a Windows `.exe` folder or a Linux folder / tarball. Everything here works from the editor (**Project > Export...**) and from the command line (`lumos ...`), and both use the same code, so the results are identical.

## How it works

An exported app is a **prebuilt Lumos Runtime** (an *export template*) plus your project:

1. **Pack** - `Assets/` becomes one `Assets.lpak` (Cache/ is left out).
2. **Assemble** - the template Runtime is copied, renamed to your app, and gets your `.lmproj`, `Assets.lpak`, icon, name, bundle id and version.
3. **Sign / zip** - codesign on macOS, Xcode signing for iOS, optional signtool on Windows, optional zip / tar.

Your project's own code is Lua, so nothing of yours is compiled - which is why you **don't need the engine source** to export. You need the editor and the templates for its version.

The Runtime finds its project at startup: `Contents/Resources/` on macOS, the bundle (or `Game/` in it) on iOS, `data/` beside the executable on Windows and Linux.

## What each platform needs

| Export to | Export from | You also need |
|---|---|---|
| macOS | a Mac | nothing for a local build. For other people's Macs: a *Developer ID Application* certificate, and notarising (`xcrun notarytool`) |
| iOS | a Mac | Xcode and an Apple developer account (free works for your own devices). Set your Team ID in the preset |
| Windows | Windows | nothing. Optional: a code signing certificate (signtool) |
| Linux | Linux | nothing |

## Export templates

Templates are prebuilt Runtimes, one per platform and config, stored per engine version:

```
<templates>/<engine version>/<platform>/<release|debug>/
```

- **release** - the shipping build (Production config): no dev tools, smallest.
- **debug** - an optimised build that keeps logging and asserts, for testing exports.

Where `<templates>` is: `$LUMOS_TEMPLATES` if set, else `Templates/` in the engine source, else `Templates/` beside the editor, else your user data folder (`~/Library/Application Support/Lumos/Templates`, `%APPDATA%/Lumos/Templates`, `~/.local/share/lumos/templates`).

**With the engine source**, build them yourself (Export window > *Build Template*, or):

```
lumos templates build --platform=macos
lumos templates build --platform=ios
lumos templates build --platform=macos --config=debug
lumos templates list
```

`lumos` is the editor in command-line mode, so **build the editor first** (`./Scripts/MacOS/BuildMacOS-Editor.sh`). The first template build compiles the whole engine and takes a while; later ones are incremental, except that a changed header needs a clean one (`rm -rf bin-int/lumos-ios-template-*`) because the projects don't track header dependencies. The iOS template build regenerates the shared Xcode projects for iOS while it runs and puts the macOS ones back when it's done.

**After any engine C++ change, rebuild the template before exporting.** Exports never compile the engine, so an old template means the phone gets the old engine. The full order is in [Building.md](Building.md#3-i-changed-engine-c-and-want-it-on-the-phone): editor, then template, then deploy.

**Without the source**, use an editor package that carries them: `Scripts/MacOS/PackageEditor.sh` builds `Lumos Editor.app` (and a DMG) with the engine assets, these docs and the templates in `Contents/Resources/`, so exporting works on a Mac that has never seen the engine source. Or copy a templates folder for your editor's version into one of the places above. `LUMOS_NO_SOURCE=1` makes a source-built editor behave like a packaged one, for testing.

## Presets

Each project keeps its export settings in `ExportPresets.json` next to the `.lmproj` (commit it). A preset is a named target - you can have several per platform (say *macOS* and *macOS Notarised*). Anything left empty comes from Project Settings.

| Field | Meaning |
|---|---|
| `Name` | shown in the Export window, used with `--preset=` |
| `Platform` | `macos`, `ios`, `windows`, `linux` |
| `Config` | `release` or `debug` (which template) |
| `OutputDir` | where the app goes, relative to the project. Default `Builds/<preset name>/` |
| `AppName` | the app's name. Default: the project Title |
| `BundleId` | e.g. `com.you.game`. Default: the project's Bundle Identifier |
| `IconPath` | a square PNG (1024px). Default: the project's Icon, else `Assets/Textures/icon.png` (iOS prefers `icon_ios.png`) |
| `Version` / `BuildNumber` | default from Project Settings |
| `SignIdentity` | macOS: `-` (ad hoc, runs on this Mac) or `Developer ID Application: Name (TEAM)`. Windows: certificate subject for signtool |
| `Team` | iOS: your Apple team id (Xcode > Settings > Accounts) |
| `Notarize` / `NotaryProfile` | macOS: notarise and staple. Store credentials once with `xcrun notarytool store-credentials <profile>` |
| `Zip` | also produce a `.zip` (Windows, macOS) or `.tar.gz` (Linux) |
| `Device` | iOS: device name to deploy to |
| `Upload` | `archive` sends the build to App Store Connect instead of writing an `.ipa` / `.pkg` |
| `Category` | macOS: `LSApplicationCategoryType`, default `public.app-category.games` |
| `Exclude` | list of strings: any `//Assets/...` path containing one stays out of the pack (save files, test data) |

Build output goes to `Builds/`, intermediates to `.lumos/` - add both to your project's `.gitignore`.

Project files the exporter picks up when present:

- `Package.json` - `DocumentTypes` for Finder / Files "Open With"; `InfoPlist` (`{ "Key": value }`) sets any other Info.plist keys on macOS and iOS.
- `macOS/<Name>.entitlements` - used when signing with a real identity.
- `iOS/Info.plist` - replaces the stock iOS Info.plist (background audio etc.).
- `PrivacyInfo.xcprivacy` - App Store privacy manifest (macOS + iOS).

## The Export window

**Project > Export...**

- Presets on the left: add one per platform, duplicate, delete. Changes save to `ExportPresets.json` as you go.
- Settings for the selected preset on the right, with the values they fall back to shown greyed.
- The template status for the preset's platform, with **Build Template** when the engine source is available.
- **Export**, **Export & Run** (runs it on this machine), **Deploy** (iOS: install + launch on the chosen device), **App Store Archive** / **Upload to App Store** (iOS), **Open Folder**.
- The log streams every step and tool, with a progress bar and **Cancel**.
- The equivalent `lumos` command for the current preset, with a copy button.

## Command line

`lumos` is the editor binary in command-line mode - no window opens. From a source checkout use `Scripts/lumos` (macOS / Linux) or `Scripts\lumos.bat` (Windows); they find the newest editor build (or set `LUMOS_EDITOR`). Anywhere else, run the editor executable with the same arguments.

```
lumos help
lumos export   --project=<dir> --preset=<name>
lumos export   --project=<dir> --platform=macos [--config=debug] [--out=<dir>] [--run]
lumos deploy   --project=<dir> [--preset=iOS] [--device="Joe's iPhone"] [--team=ABCDE12345]
lumos archive  --project=<dir> [--preset=iOS|macOS] [--upload]
lumos run      --project=<dir>
lumos pack     --project=<dir> [--out=<file>]
lumos icons    --project=<dir> [--out=<dir>]
lumos devices
lumos presets  --project=<dir> [--init]
lumos templates list | path | build --platform=<p> [--config=debug|all]
```

- `export` - with `--preset`, that preset from `ExportPresets.json` (a platform name picks that platform's first preset); with `--platform`, that platform's preset or the defaults. `--run` launches it afterwards.
- `deploy` - iOS: export, then install and launch on a connected device (the first one if `--device` isn't given).
- `archive` - an App Store archive (Release, with symbols): an `.ipa` for iOS, a `.pkg` for the Mac App Store, in `Builds/<preset>/AppStore/`. With `--upload` (or `Upload` in the preset) it goes straight to App Store Connect / TestFlight instead. Uses your Xcode account and the preset's `Team`; the app has to exist in App Store Connect first. Mac App Store apps are sandboxed: the project's `macOS/<Name>.entitlements` is used, else the sandbox alone. The Mac App Store template (prebuilt universal engine libraries) is built with the macOS release template.
- `run` - runs the project from its loose files with the template Runtime, for a quick check without packing.
- `pack` - just `Assets.lpak` (default `.lumos/Assets.lpak`).
- `icons` - writes every platform's icon files from the project icon.
- `devices` - connected iOS devices, as names you can pass to `--device`.
- `presets --init` - writes an `ExportPresets.json` with one preset per platform this machine can export.

Every command exits non-zero on failure, so they work in scripts and CI.

Examples:

```
Scripts/lumos export --project=Tools/LumosTunes --platform=macos --run
Scripts/lumos deploy --project=Tools/LumosTunes --device="Joseph's iPhone"
Scripts/lumos export --project=../Orion --preset="macOS Notarised"
```

## Troubleshooting

- **"No macOS release template"** - build it (`lumos templates build --platform=macos`) or install templates for this engine version.
- **iOS: "No Team set" / signing errors** - set `Team` in the iOS preset. The first export to a new device may need Xcode to register it: open `.lumos/<preset>/ios/LumosApp.xcodeproj` once and let it fix signing.
- **iOS: "... is locked"** - unlock the phone and run Deploy again.
- **iOS: "No device called ..."** - `lumos devices` shows the names it can see; the phone has to be paired and trusted.
- **macOS: "is damaged and can't be opened" on another Mac** - ad-hoc signed apps only run where they were built. Use a Developer ID identity and notarise.
- **The app opens but shows the wrong project / nothing** - check the log's "Packed ... KB" line and that the project's `Assets/` holds what the game loads; exported apps read only `Assets.lpak`.
- **Templates seem stale after engine changes** - rebuild them; exports never compile the engine. If you changed a header, clear `bin-int/lumos-ios-template-*` first.
- **The editor crashed during *Build Template*** - older editors could overflow the text renderer on long compiler lines in the log. Rebuild the editor, or build the template from the command line (`Scripts/lumos templates build --platform=ios`).
- **A macOS build after an iOS template build targets the wrong platform** - older template builds left the shared Xcode projects set up for iOS. Run `./Scripts/MacOS/GenerateXCode.sh` (current builds restore it themselves).
- **`lumos: no LumosEditor build found`** - build the editor first; `lumos` runs it.
