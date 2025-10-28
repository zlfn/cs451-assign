# Create 1_2d_game executable
add_gl_executable_single_file(2_2d_animation src/main.cpp)

target_sources(2_2d_animation PRIVATE
    src/TrailParticle.cpp
    src/Bullet.cpp
    src/BulletPattern.cpp
    src/Player.cpp
    src/BossMove.cpp
    src/MovePattern.cpp
    src/Boss.cpp
    src/HealthBar.cpp
    src/BossIndicator.cpp
    src/Background.cpp
    src/CommandExecuter.cpp
    src/utils.cpp
)

# Add local include directories for GLM and other headers
target_include_directories(2_2d_animation PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../win-x64-msvc/include
)
