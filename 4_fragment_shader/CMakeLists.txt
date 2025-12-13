add_gl_executable_single_file(4_fragment_shader src/main.cpp)

# Include shader conversion script
include(cmake/ShaderToHeader.cmake)

# Collect all shader files
file(GLOB SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.comp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.glsl
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
add_custom_target(proj4_generate_shaders DEPENDS ${SHADER_HEADER_OUTPUT})

# Copy assets to bin directory (where executable is located)
add_custom_target(proj4_copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/bin/assets
    COMMENT "Copying assets to bin directory"
)

# Link all project source files to the executable target
target_sources(4_fragment_shader PRIVATE
    src/graphics.cpp
    src/TrailParticle.cpp
    src/Bullet.cpp
    src/BulletPattern.cpp
    src/Player.cpp
    src/BossMove.cpp
    src/MovePattern.cpp
    src/Boss.cpp
    src/HealthBar.cpp
    src/Background.cpp
    src/CommandExecuter.cpp
    src/utils.cpp
    src/Skybox.cpp
    src/LightSource.cpp
    src/PostProcess.cpp
    src/ShadowMap.cpp
)

# Make sure shaders and assets are ready before building
add_dependencies(4_fragment_shader proj4_generate_shaders proj4_copy_assets)

# Define macro to enable generated shaders
target_compile_definitions(4_fragment_shader PRIVATE SHADERS_GENERATED)

# Add local include directories for headers
target_include_directories(4_fragment_shader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../win-x64-msvc/include
    ${stb_SOURCE_DIR}
)
