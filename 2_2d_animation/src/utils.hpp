#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <numbers>
#include <algorithm>
#include <random>

extern std::random_device rd;
extern std::mt19937 gen;
extern std::uniform_real_distribution<float> dist;

int getRandomRange(int a, int b);
void drawSpaceship(glm::fvec2 center, float size, glm::fvec4 color);
void drawRectWithGlow(float x, float y, float width, float height, glm::fvec4 color, float glowSize,
                      float zDepth);

constexpr bool approxEqual(float a, float b, float epsilon = 1e-2f) {
    return (a > b ? a - b : b - a) < epsilon;
}

/// @brief Concept for functions f(0)=f(1)=0
/// @tparam F Function type
template <typename F>
concept Map00Fn = requires {
    { F{}(0.0f) } -> std::same_as<float>;
    { F{}(1.0f) } -> std::same_as<float>;
} && approxEqual(F{}(0.0f), 0.0f) && approxEqual(F{}(1.0f), 0.0f);

/// @brief Concept for functions f(0)=0, f(1)=1
/// @tparam F Function type
template <typename F>
concept Map01Fn = requires {
    { F{}(0.0f) } -> std::same_as<float>;
    { F{}(1.0f) } -> std::same_as<float>;
} && approxEqual(F{}(0.0f), 0.0f) && approxEqual(F{}(1.0f), 1.0f);
