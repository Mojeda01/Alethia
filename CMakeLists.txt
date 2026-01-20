cmake_minimum_required(VERSION 3.21)
project(VulkanLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

#if 0
add_executable(VulkanLab 
    src/main.cpp
    src/VulkanApp.cpp
    src/Window.cpp
    src/Instance.cpp
    src/Surface.cpp
    src/Device.cpp
    src/Swapchain.cpp
    src/RenderPass.cpp
    src/Framebuffer.cpp
    src/CommandPool.cpp
    src/SyncObjects.cpp
    src/SwapchainBundle.cpp
)
#endif
target_link_libraries(VulkanLab PRIVATE Vulkan::Vulkan glfw)
