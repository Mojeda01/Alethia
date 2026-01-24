# Alethia

[![Website](https://img.shields.io/badge/website-marco--oj.no-0b7285)](https://marco-oj.no/)
![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.21%2B-064F8C?logo=cmake&logoColor=white)
![Vulkan](https://img.shields.io/badge/Vulkan-1.3-AC162C?logo=vulkan&logoColor=white)
![Status](https://img.shields.io/badge/status-WIP-yellow)

A minimal Vulkan playground: GLFW window + swapchain + render pass + a basic triangle pipeline.

## Requirements
- CMake 3.21+
- C++20 compiler
- Vulkan SDK (for headers/loader + `glslc` or `glslangValidator`)
- GLFW3

Note: macOS typically uses MoltenVK; this project enables Vulkan portability extensions.

## Build & run
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/VulkanLab
