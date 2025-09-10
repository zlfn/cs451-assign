# Create 0_setup executable
add_gl_executable_single_file(0_setup msvc/testbed.cpp)

# Add local include directories for GLM and other headers
target_include_directories(0_setup PRIVATE ../win-x64-msvc/include)
