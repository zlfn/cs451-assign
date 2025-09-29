# Set C++20 standard for modules
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable C++20 modules support
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Enable experimental module support for Clang
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # LLVM Clang 21+ uses standard C++20 modules
    add_compile_options(-stdlib=libc++)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    add_compile_options(-fmodules-ts -fbuiltin-module-map)
endif()

# Create modules
add_library(game_collision)
target_sources(game_collision
    PUBLIC
        FILE_SET CXX_MODULES FILES
        src/collision.cppm
)
set_target_properties(game_collision PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_include_directories(game_collision PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../win-x64-msvc/include
)

add_library(game_utils)
target_sources(game_utils
    PUBLIC
        FILE_SET CXX_MODULES FILES
        src/utils.cppm
)
set_target_properties(game_utils PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_include_directories(game_utils PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../win-x64-msvc/include
)

# Create 2_2d_animation executable
add_gl_executable_single_file(2_2d_animation src/main.cpp)

# Link modules and set include directories
target_link_libraries(2_2d_animation game_collision game_utils)
target_include_directories(2_2d_animation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../win-x64-msvc/include
)
