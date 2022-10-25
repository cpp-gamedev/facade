# facade

**A native 3D GLTF scene viewer and editor (WIP)**

## Requirements

### Runtime

- Vulkan 1.1 loader, conformant / portability-compatible ICD
- C++ runtime (linked dynamically)
- Desktop OS
  - Linux
  - Windows
  - MacOS [experimental]

### Build-time

- CMake 3.18+
- C++20 compiler and standard library
- Vulkan 1.1+ SDK
- Desktop windowing system/manager
  - Primary development environments:
    - Plasma KDE / X11
    - Windows 10
    - Raspbian (aarch64)
  - Other supported environments:
    - GNOME, Xfce, Wayland, ... [untested]
    - Windows 11/8/7/... [untested]
    - MacOS (through MoltenVK / Vulkan SDK) [experimental]

## Building

Using VSCode with CMake Tools or Visual Studio CMake is recommended: load a desired preset, configure, build, and debug/run. Modify the [launch.json](https://github.com/cpp-gamedev/facade/wiki/Development#vscode-launchjson-template) template provided in the wiki and place it in `.vscode/` for customized debugging.

WIP
