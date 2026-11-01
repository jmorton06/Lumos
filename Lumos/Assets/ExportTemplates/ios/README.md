iOS export template project. The exporter copies this next to a generated App.xcconfig (name, bundle id,
team, orientation...), Info.plist, Assets.xcassets, Launch Screen.storyboard and Game/ (the .lmproj +
Assets.lpak), then builds it against the prebuilt engine libraries in the template's lib/ folder.
