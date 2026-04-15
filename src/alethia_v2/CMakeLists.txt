add_library(alethia_v2 STATIC
    initv2.cpp
    vkInstance.cpp
    VkLogicalDevice.cpp
    VkPhysicalDevice.cpp
    VkPipeline.cpp
    VkRenderer.cpp
    VkSwapchain.cpp
)

target_include_directories(alethia_v2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(alethia_v2 PUBLIC
    Vulkan::Vulkan
    glfw
)

target_compile_definitions(alethia_v2 PRIVATE
    ALETHIA_V2_SHADER_DIR="${CMAKE_BINARY_DIR}/shaders"
)

add_dependencies(alethia_v2 AlethiaV2Shaders)
target_compile_definitions(alethia_v2 PRIVATE
    ALETHIA_V2_SHADER_DIR="${CMAKE_BINARY_DIR}/shaders"
)

