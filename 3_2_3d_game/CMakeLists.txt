# Create 3_2_3d_game executable
add_gl_executable_single_file(3_2_3d_game src/main.cpp)

target_sources(3_2_3d_game PRIVATE
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
)

# Add local include directories for GLM and other headers
target_include_directories(3_2_3d_game PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../win-x64-msvc/include
    ${stb_SOURCE_DIR}
)
