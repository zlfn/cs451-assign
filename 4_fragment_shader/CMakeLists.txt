# Create 4_fragment_shader executable
add_gl_executable_single_file(4_fragment_shader src/main.cpp)

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
)

# Add local include directories for GLM and other headers
target_include_directories(4_fragment_shader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../win-x64-msvc/include
    ${stb_SOURCE_DIR}
)
