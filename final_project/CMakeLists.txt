# Create final_project executable
add_gl_executable_single_file(final_project src/main.cpp)

target_sources(final_project PRIVATE
    src/utils.cpp
)

# Add local include directories for GLM and other headers
target_include_directories(final_project PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../win-x64-msvc/include
    ${stb_SOURCE_DIR}
)
