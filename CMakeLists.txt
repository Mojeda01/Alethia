cmake_minimum_required(VERSION 3.21)
project(VulkanLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

# Shader compilation
set(SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_BIN_DIR})

find_program(GLSLC glslc)
find_program(GLSLANG_VALIDATOR glslangValidator)

if (GLSLC)
    set(SHADER_COMPILER ${GLSLC})
    set(SHADER_COMPILER_ARGS)
elseif (GLSLANG_VALIDATOR)
    set(SHADER_COMPILER ${GLSLANG_VALIDATOR})
    set(SHADER_COMPILER_ARGS -V)
else()
    message(FATAL_ERROR "No shader compiler found. Install glslc or glslangValidator (typically from the Vulkan SDK).")
endif()

set(TRI_VERT_SPV ${SHADER_BIN_DIR}/triangle.vert.spv)
set(TRI_FRAG_SPV ${SHADER_BIN_DIR}/triangle.frag.spv)

add_custom_command(
    OUTPUT ${TRI_VERT_SPV}
    COMMAND ${SHADER_COMPILER} ${SHADER_COMPILER_ARGS} ${SHADER_SRC_DIR}/triangle.vert -o ${TRI_VERT_SPV}
    DEPENDS ${SHADER_SRC_DIR}/triangle.vert
    VERBATIM
)

add_custom_command(
    OUTPUT ${TRI_FRAG_SPV}
    COMMAND ${SHADER_COMPILER} ${SHADER_COMPILER_ARGS} ${SHADER_SRC_DIR}/triangle.frag -o ${TRI_FRAG_SPV}
    DEPENDS ${SHADER_SRC_DIR}/triangle.frag
    VERBATIM
)

add_custom_target(VulkanLabShaders DEPENDS ${TRI_VERT_SPV} ${TRI_FRAG_SPV})

add_library(VulkanTriangle STATIC
    src/triangle/TriangleRenderer.cpp
)
target_compile_definitions(VulkanTriangle PRIVATE VULKANLAB_SHADER_DIR="${SHADER_BIN_DIR}")
target_link_libraries(VulkanTriangle PUBLIC Vulkan::Vulkan)

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
    src/FrameSync.cpp
    src/Buffer.cpp
    src/MeshBuffer.cpp
    src/DepthImage.cpp
    src/UniformBuffer.cpp
    src/Camera.cpp
    src/ObjLoader.cpp
)

add_dependencies(VulkanLab VulkanLabShaders)
target_link_libraries(VulkanLab PRIVATE Vulkan::Vulkan glfw VulkanTriangle)
