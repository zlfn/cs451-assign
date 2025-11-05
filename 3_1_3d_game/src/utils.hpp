#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <numbers>
#include <algorithm>
#include <random>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

extern std::random_device rd;
extern std::mt19937 gen;
extern std::uniform_real_distribution<float> dist;

int getRandomRange(int a, int b);
void drawRectWithGlow(float x, float y, float width, float height, glm::fvec4 color, float glowSize,
                      float zDepth);

constexpr bool approxEqual(float a, float b, float epsilon = 1e-2f) {
    return (a > b ? a - b : b - a) < epsilon;
}

// f(0)=f(1)=0을 만족하는 함수의 콘셉트
template <typename F>
concept Map00Fn = requires {
    { F{}(0.0f) } -> std::same_as<float>;
    { F{}(1.0f) } -> std::same_as<float>;
} && approxEqual(F{}(0.0f), 0.0f) && approxEqual(F{}(1.0f), 0.0f);

// f(0)=0, f(1)=1을 만족하는 함수의 콘셉트
template <typename F>
concept Map01Fn = requires {
    { F{}(0.0f) } -> std::same_as<float>;
    { F{}(1.0f) } -> std::same_as<float>;
} && approxEqual(F{}(0.0f), 0.0f) && approxEqual(F{}(1.0f), 1.0f);

using Indices = std::vector<unsigned int>;

struct ThreeDObj {
    std::vector<glm::vec3> baseVertices;
    std::map<std::string, Indices> objIndicesMap;
    std::map<std::string, glm::vec3> objCenterMap;
    glm::vec3 objectColor;

    ThreeDObj(const std::string &FILE_PATH, const glm::fvec3 &color = glm::fvec3(1.0f, 1.0f, 1.0f));

    void getObjFile(const std::string &FILE_PATH);
    void setColor(const glm::vec3 &color);
    void draw(const std::string objName);
};
