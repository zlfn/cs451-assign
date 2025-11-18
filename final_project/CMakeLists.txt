# Include shader conversion script
include(cmake/ShaderToHeader.cmake)

# Collect all shader files
file(GLOB SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.frag
)

# Generate shader header - only at build time with dependency tracking
set(SHADER_HEADER_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders_generated.hpp)

# Add custom command to regenerate shaders when they change
add_custom_command(
    OUTPUT ${SHADER_HEADER_OUTPUT}
    COMMAND ${CMAKE_COMMAND}
        -DSHADER_HEADER_OUTPUT=${SHADER_HEADER_OUTPUT}
        -DSHADER_DIR=${CMAKE_CURRENT_SOURCE_DIR}/src/shaders
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateShaders.cmake
    DEPENDS ${SHADER_FILES}
    COMMENT "Generating shader header from shader files"
    VERBATIM
)

# Add custom target to ensure shader generation runs
add_custom_target(generate_shaders DEPENDS ${SHADER_HEADER_OUTPUT})

# Create final_project executable with GLFW
add_glfw_executable_single_file(final_project src/main.cpp)

target_sources(final_project PRIVATE
    src/utils.cpp
)

# Make sure shaders are generated before building
add_dependencies(final_project generate_shaders)

# Define macro to enable generated shaders
target_compile_definitions(final_project PRIVATE SHADERS_GENERATED)

# Add local include directories for GLM and other headers
target_include_directories(final_project PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}  # For generated shader header
    ${CMAKE_CURRENT_SOURCE_DIR}/../win-x64-msvc/include
    ${stb_SOURCE_DIR}
)
