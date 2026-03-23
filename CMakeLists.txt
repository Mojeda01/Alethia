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

set(GRID_VERT_SPV ${SHADER_BIN_DIR}/grid.vert.spv)
set(GRID_FRAG_SPV ${SHADER_BIN_DIR}/grid.frag.spv)

add_custom_command(
    OUTPUT ${GRID_VERT_SPV}
    COMMAND ${SHADER_COMPILER} ${SHADER_COMPILER_ARGS} ${SHADER_SRC_DIR}/grid.vert -o ${GRID_VERT_SPV}
    DEPENDS ${SHADER_SRC_DIR}/grid.vert
    VERBATIM
)

add_custom_command(
    OUTPUT ${GRID_FRAG_SPV}
    COMMAND ${SHADER_COMPILER} ${SHADER_COMPILER_ARGS} ${SHADER_SRC_DIR}/grid.frag -o ${GRID_FRAG_SPV}
    DEPENDS ${SHADER_SRC_DIR}/grid.frag
    VERBATIM
)

set(GIZMO_VERT_SPV ${SHADER_BIN_DIR}/gizmo.vert.spv)
set(GIZMO_FRAG_SPV ${SHADER_BIN_DIR}/gizmo.frag.spv)

add_custom_command(
    OUTPUT ${GIZMO_VERT_SPV}
    COMMAND ${SHADER_COMPILER} ${SHADER_COMPILER_ARGS} ${SHADER_SRC_DIR}/gizmo.vert -o ${GIZMO_VERT_SPV}
    DEPENDS ${SHADER_SRC_DIR}/gizmo.vert
    VERBATIM
)

add_custom_command(
    OUTPUT ${GIZMO_FRAG_SPV}
    COMMAND ${SHADER_COMPILER} ${SHADER_COMPILER_ARGS} ${SHADER_SRC_DIR}/gizmo.frag -o ${GIZMO_FRAG_SPV}
    DEPENDS ${SHADER_SRC_DIR}/gizmo.frag
    VERBATIM
)

set(LINE_VERT_SPV ${SHADER_BIN_DIR}/line.vert.spv)
set(LINE_FRAG_SPV ${SHADER_BIN_DIR}/line.frag.spv)

add_custom_command(
    OUTPUT ${LINE_VERT_SPV}
    COMMAND ${SHADER_COMPILER} ${SHADER_COMPILER_ARGS} ${SHADER_SRC_DIR}/line.vert -o ${LINE_VERT_SPV}
    DEPENDS ${SHADER_SRC_DIR}/line.vert
    VERBATIM
)

add_custom_command(
    OUTPUT ${LINE_FRAG_SPV}
    COMMAND ${SHADER_COMPILER} ${SHADER_COMPILER_ARGS} ${SHADER_SRC_DIR}/line.frag -o ${LINE_FRAG_SPV}
    DEPENDS ${SHADER_SRC_DIR}/line.frag
    VERBATIM
)

add_custom_target(
    VulkanLabShaders DEPENDS ${TRI_VERT_SPV} ${TRI_FRAG_SPV} ${GRID_VERT_SPV} ${GRID_FRAG_SPV} ${GIZMO_VERT_SPV} ${GIZMO_FRAG_SPV} ${LINE_VERT_SPV} ${LINE_FRAG_SPV})

add_library(VulkanTriangle STATIC
    src/triangle/TriangleRenderer.cpp
    src/GridRenderer.cpp
    src/LineRenderer.cpp
)
target_compile_definitions(VulkanTriangle PRIVATE VULKANLAB_SHADER_DIR="${SHADER_BIN_DIR}")
target_include_directories(VulkanTriangle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
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
    src/TextureImage.cpp 
    src/ImGuiLayer.cpp
    src/DebugUI.cpp
    src/Log.cpp
    src/SceneEditor.cpp
    src/LineBatch.cpp
    src/InputManager.cpp
    src/GizmoRenderer.cpp
    src/GizmoMesh.cpp
    src/CubeMesh.cpp
    src/DevTexture.cpp
    src/PhysicsSolver.cpp
    src/PlayerController.cpp
    src/ColorPalette.cpp
    src/MaterialPanel.cpp
    src/external/imgui/imgui.cpp
    src/external/imgui/imgui_draw.cpp
    src/external/imgui/imgui_tables.cpp
    src/external/imgui/imgui_widgets.cpp
    src/external/imgui/imgui_demo.cpp
    src/external/imgui/imgui_impl_glfw.cpp
    src/external/imgui/imgui_impl_vulkan.cpp
)

target_compile_definitions(VulkanLab PRIVATE VULKANLAB_SHADER_DIR="${SHADER_BIN_DIR}")
add_dependencies(VulkanLab VulkanLabShaders)
target_include_directories(VulkanLab PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/external/imgui)
target_link_libraries(VulkanLab PRIVATE Vulkan::Vulkan glfw VulkanTriangle)
