# Helios3D Engine (Beta Foundation)

Helios3D Engine is a modular C++ 3D editor foundation with a clean separation between core runtime, rendering, scene data, and editor tooling.

## Implemented Foundation

- Engine core: window lifecycle, timing, logging, clean init/shutdown.
- Renderer: OpenGL pipeline, camera, scene render loop, stats tracking.
- Scene system: entities, transforms, materials, hierarchy links, selection API.
- Editor: ImGui dockspace layout, hierarchy, inspector, viewport, assets panel, stats panel, console panel.
- Editing workflows: multi-select, duplicate (`Ctrl+D`), focus selected (`F`), hide/unhide (`H`), save/load scene, recent files, autosave + recovery.
- QoL: notifications, wireframe toggle infrastructure, scene serialization to JSON.

## Folder Structure

`src/core` - app/runtime systems  
`src/renderer` - camera and render backend  
`src/scene` - scene/entity/model serialization  
`src/editor` - editor panels and interactions  
`assets/shaders` - shader files  
`assets/scenes` - scene JSON files and autosave/recent data

## Build (Windows)

### Prerequisites

- CMake 3.24+
- Visual Studio 2022 (C++ workload) or another C++20 toolchain
- Git (for FetchContent dependencies)

### Commands

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Executable output is generated in the build folder (Visual Studio generators place it under `build/Release`).

## Notes

- The project uses `FetchContent` for GLFW, GLAD, ImGui, ImGuizmo, GLM, and nlohmann/json.
- Assets are copied to the build output by CMake.
