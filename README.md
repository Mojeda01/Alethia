<p align="center">
  <img src="screenshots/logo_4.png" alt="Alethia" width="600" />
</p>

<p align="center">
  <a href="https://marco-oj.no"><img src="https://img.shields.io/badge/website-marco--oj.no-blue" alt="website"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue" alt="C++">
  <img src="https://img.shields.io/badge/CMake-3.21+-orange" alt="CMake">
  <img src="https://img.shields.io/badge/Vulkan-1.3-red" alt="Vulkan">
  <img src="https://img.shields.io/badge/status-WIP-yellow" alt="status">
  <br>
  <img src="https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Windows-lightgrey" alt="platform">
  <img src="https://img.shields.io/badge/GPU-MoltenVK%20%7C%20AMD%20%7C%20NVIDIA-green" alt="GPU">
  <img src="https://img.shields.io/badge/renderer-custom%20Vulkan-blueviolet" alt="renderer">
  <img src="https://img.shields.io/badge/shading-Blinn--Phong-informational" alt="shading">
  <br>
  <img src="https://img.shields.io/badge/textures-stb__image-orange" alt="textures">
  <img src="https://img.shields.io/badge/models-tinyobjloader-orange" alt="models">
  <img src="https://img.shields.io/badge/UI-Dear%20ImGui-blue" alt="ImGui">
  <img src="https://img.shields.io/badge/math-GLM-purple" alt="GLM">
  <img src="https://img.shields.io/badge/windowing-GLFW3-green" alt="GLFW">
  <img src="https://img.shields.io/badge/physics-AABB-blue" alt="physics">
  <img src="https://img.shields.io/badge/editor-level%20editor-brightgreen" alt="editor">
  <img src="https://img.shields.io/badge/undo%2Fredo-64%20steps-informational" alt="undo">
  <br>
  <img src="https://img.shields.io/badge/license-proprietary-red" alt="license">
</p>

A custom Vulkan rendering engine and level editor built from scratch in C++20. Features Blinn-Phong lighting with hemisphere ambient, specular, and rim lighting, OBJ model loading with texture support, and a procedural grid renderer. The block placement system supports click-drag sizing, surface-aware stacking, face-drag resizing, axis-aligned slicing, multi-select, copy/paste with paste preview, and a 64-step undo/redo history. Scenes are saved and loaded in a custom binary format. Includes a modular debug UI with collapsible panels, a runtime logging system, line rendering infrastructure, configurable grid snapping, and an orientation gizmo. Per-object transforms via push constants, independent input management, and multiple rendering pipelines running in a single render pass. Features an edit/play mode switch with a first-person player controller, per-axis swept AABB collision against scene geometry, gravity, jumping, sprinting, and noclip flight mode.

![Alethia Screenshot](screenshots/alethia_18_03_2026.png)
![Alethia Screenshot](screenshots/alethia_5.png)

## Requirements

- CMake 3.21+
- C++20 compiler
- Vulkan SDK (for headers/loader + `glslc` or `glslangValidator`)
- GLFW3
- GLM

> **macOS:** MoltenVK is required. This project enables Vulkan portability
extensions automatically. GLM is expected at `/opt/homebrew/include/glm/`
(install via `brew install glm`).

## Build & run
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/VulkanLab
```

## Controls

### Editor Mode
- **WASD** — fly camera
- **Mouse** — look
- **Space** — fly up
- **Shift** — fly down
- **Tab** — toggle editor UI / cursor
- **Escape** — quit

### Editor Tools (UI mode)
- **1** — Place tool
- **2** — Select tool
- **3** — Slice tool
- **4** — Move tool
- **Del / Backspace** — delete selected cube
- **Shift+Click** — multi-select
- **CMD+C** — copy selection
- **CMD+V** — paste
- **CMD+Z** — undo
- **CMD+Shift+Z** — redo
- **Shift+Drag** — move cube on Y axis

### Play Mode (P to enter, P to return to editor)
- **WASD** — walk
- **Shift** — sprint
- **Space** — jump
- **Mouse** — look
- **F** — toggle noclip / free flight
- **P** — return to editor

![Alethia Screenshot](screenshots/aleitha_2.png)
![Alethia Screenshot](screenshots/alethia_4.png)
