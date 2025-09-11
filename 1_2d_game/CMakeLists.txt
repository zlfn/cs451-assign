# Create 1_2d_game executable
add_gl_executable_single_file(1_2d_game src/main.cpp)

# Add local include directories for GLM and other headers
target_include_directories(1_2d_game PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../win-x64-msvc/include
)

# Create test executable for collision detection
add_executable(test_collision tests/test_collision.cpp)
target_include_directories(test_collision PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../win-x64-msvc/include
)

# Add test to CTest
enable_testing()
add_test(NAME CollisionDetectionTest COMMAND test_collision)
