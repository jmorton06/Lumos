# ImGui Command Palette

![screenshot1](https://user-images.githubusercontent.com/36975818/146656302-646eccfd-6bf4-4ad0-80e0-239c7766210a.png)

## About
This library implements a Sublime Text or VSCode style command palette in ImGui.
It is provided in the form of a window that you can choose to open/close based on condition (for example, when user pressed the shortcut Ctrl+Shift+P).

## Features
+ Minimum C++ 11
+ Dynamic registration and unregistration of commands
+ Subcommands (prompting a new set of options after user selected a top-level command)
+ Fuzzy search of commands and subcommands
    + Highlighting of matched characters
        + Option: setting custom font
        + Option: setting custom text color


## Planned Features
+ [ ] Support for std::string_view
+ [ ] Support for function pointers instead of std::function
+ [ ] Visualization of previously entered options (example: Sublime Merge)
+ [x] Highlighting of matched characters using underline
+ [ ] Command history
+ [x] Reducing the minimum required C++ version

## Usage
Simply drop all .h and .cpp files in the project root folder to your buildsystem. Minimum of C++11 is required.
No external dependencies except Dear ImGui are required.

See the examples for how to use the APIs.

## Demo
This project provides examples located in the `examples/` folder. All dependencies are retreived with conan and built with CMake.
One option to build the examples:
```
# In examples/
$ mkdir build
$ cd build
$ conan install .. -s build_type=Debug
$ cmake .. -GNinja
$ ninja
```

Note: run the examples with working directory = `${projectFolder}/examples`, since they use relative paths to locate the fonts
