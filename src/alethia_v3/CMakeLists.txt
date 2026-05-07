# src/alethia_v3/CMakeLists.txt

# --- Shader compilation (v3 only) ---
set(V3_SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(V3_SHADER_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${V3_SHADER_BIN_DIR})

find_program(GLSLC glslc)
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found. Install Vulkan SDK.")
endif()

# Add your v3 shaders here (one pair per shader for now)
set(V3_SHADERS
    # Example — replace/add your actual files:
    # triangle.vert triangle.frag
    # grid.vert grid.frag
    # etc.
)

set(V3_SPV_FILES "")
foreach(SHADER ${V3_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    get_filename_component(SHADER_EXT  ${SHADER} EXT)

    set(SPV_OUTPUT ${V3_SHADER_BIN_DIR}/${SHADER_NAME}${SHADER_EXT}.spv)

    add_custom_command(
        OUTPUT ${SPV_OUTPUT}
        COMMAND ${GLSLC} ${V3_SHADER_SRC_DIR}/${SHADER} -o ${SPV_OUTPUT}
        DEPENDS ${V3_SHADER_SRC_DIR}/${SHADER}
        VERBATIM
    )
    list(APPEND V3_SPV_FILES ${SPV_OUTPUT})
endforeach()

add_custom_target(AlethiaV3Shaders DEPENDS ${V3_SPV_FILES})

# --- Library ---
add_library(alethia_v3 STATIC
    initv3.cpp
    AlethiaV3.cpp
    Vulkan/createInstance.cpp
    initVulkan.cpp
    # Add your .cpp files here as you create them
)

target_include_directories(alethia_v3 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/external/imgui
)

target_link_libraries(alethia_v3 PUBLIC
    Vulkan::Vulkan
    glfw
)

if(APPLE)
    target_link_libraries(alethia_v3 PUBLIC
        "-framework Metal"
        "-framework Foundation"
        "-framework QuartzCore"
        "-framework CoreGraphics"
        "-framework IOSurface"
    )

    target_compile_definitions(alethia_v3 PRIVATE
        MVK_CONFIG_LOG_LEVEL=1
    )
endif()

target_compile_definitions(alethia_v3 PRIVATE
    ALETHIA_V3_SHADER_DIR="${V3_SHADER_BIN_DIR}"
)

add_dependencies(alethia_v3 AlethiaV3Shaders)
