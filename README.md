# Alethia

[![website](https://img.shields.io/badge/website-marco--oj.no-blue)](https://marco-oj.no)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue)
![CMake](https://img.shields.io/badge/CMake-3.21+-orange)
![Vulkan](https://img.shields.io/badge/Vulkan-1.3-red)
![status](https://img.shields.io/badge/status-WIP-yellow)

A custom Vulkan rendering engine built from scratch. GPU vertex buffers,
depth testing, SPIR-V shader pipeline, swapchain management with resize
handling. 

## Requirements

- CMake 3.21+
- C++20 compiler
- Vulkan SDK (for headers/loader + `glslc` or `glslangValidator`)
- GLFW3

Note: macOS typically uses MoltenVK; this project enables Vulkan portability extensions.

## Build & run
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/VulkanLab
```

## Architecture
```
src/
├── main.cpp              Entry point
├── VulkanApp.h/cpp       Application loop and frame submission
├── Window.h/cpp          GLFW window wrapper
├── Instance.h/cpp        Vulkan instance
├── Surface.h/cpp         Window surface
├── Device.h/cpp          Physical/logical device and queues
├── Swapchain.h/cpp       Swapchain images and views
├── SwapchainBundle.h/cpp Swapchain + depth image + render pass + framebuffers
├── DepthImage.h/cpp      Depth buffer image and view
├── RenderPass.h/cpp      Color + depth render pass
├── Framebuffer.h/cpp     Per-image framebuffers
├── CommandPool.h/cpp     Command pool and buffer allocation
├── FrameSync.h/cpp       Semaphores and fences for frame pacing
├── Buffer.h/cpp          General-purpose Vulkan buffer (RAII)
├── MeshBuffer.h/cpp      Staged vertex upload to device-local memory
├── Vertex.h              Vertex layout and input descriptions
└── triangle/
    └── TriangleRenderer  Shader pipeline, push constants, draw recording

shaders/
├── triangle.vert         Vertex shader (vertex attributes + time rotation)
└── triangle.frag         Fragment shader (color cycling)
```


