# Script to regenerate shaders at build time
# This is called by the custom command in CMakeLists.txt

# Include the shader conversion function
include(${CMAKE_CURRENT_LIST_DIR}/ShaderToHeader.cmake)

# Collect shader files at build time
file(GLOB SHADER_FILES_LIST
    ${SHADER_DIR}/*.vert
    ${SHADER_DIR}/*.frag
)

message(STATUS "SHADER_FILES_LIST: ${SHADER_FILES_LIST}")

# Generate the shader header
shaders_to_header(${SHADER_HEADER_OUTPUT} ${SHADER_FILES_LIST})
