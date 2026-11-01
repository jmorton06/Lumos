Mac App Store export template project. The exporter copies this next to a generated App.xcconfig (name,
bundle id, team, category, entitlements), Info.plist, Assets.xcassets (icon set) and Game/ (the .lmproj +
Assets.lpak), then archives it against the prebuilt universal engine libraries in the template's lib/ folder.
