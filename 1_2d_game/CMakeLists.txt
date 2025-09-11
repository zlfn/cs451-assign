# Create 0_setup executable
add_gl_executable_single_file(1_2d_game msvc/main.cpp)

# Add local include directories for GLM and other headers
target_include_directories(1_2d_game PRIVATE ../win-x64-msvc/include)
