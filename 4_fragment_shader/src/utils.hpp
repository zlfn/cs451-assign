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
#include <stdexcept>
#include <set>
#include <queue>
#include <utility>
#include <map>
#include <memory>
#include "graphics.hpp"

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

struct IndexInfo {
    unsigned int v_index = 0;
    unsigned int vt_index = 0;
    unsigned int vn_index = 0;

    IndexInfo() : v_index(0), vn_index(0) {}
    IndexInfo(unsigned int v_index, unsigned int vn_index) : v_index(v_index), vn_index(vn_index) {}

    bool operator<(const IndexInfo &other) const {
        if (v_index != other.v_index) {
            return v_index < other.v_index;
        }
        return vn_index < other.vn_index;
    }
};
using Indices = std::vector<IndexInfo>;

struct ThreeDObj {
    std::vector<glm::vec3> baseVertices;
    std::vector<glm::vec3> baseNormals;
    std::vector<glm::vec2> baseTexCoords;
    std::map<std::string, Indices> objIndicesMap;
    std::map<std::string, glm::vec3> objCenterMap;
    std::map<std::string, glm::vec3> objTranslationMap;
    std::map<std::string, std::unique_ptr<Mesh>> objMeshMap;
    glm::vec3 objectColor;
    GLuint textureID;
    GLuint normalMapID;

    ThreeDObj(const std::string &FILE_PATH, const std::string &TEXTURE_PATH,
              const std::string &NORMAL_PATH, const glm::fvec3 &color = glm::fvec3(1.0f, 1.0f, 1.0f));

    void getObjFile(const std::string &FILE_PATH);
    void getFragInfo(const std::string &TEXTURE_PATH, const std::string &NORMAL_PATH);
    void setColor(const glm::vec3 &color);
    void draw(const std::string objName);

    void drawAll();
    void separate(float separation_step);
    void splitObjectByZPlane(const std::string &objName, float z_plane = 0.0f);

  private:
    glm::vec3 intersectPlane(const glm::vec3 &p1, const glm::vec3 &p2, float z_plane);
    glm::vec3 calculateCenter(const Indices &indices);
    void findConnectedComponents(Indices &all_indices, const std::string &objName,
                                 const std::string &suffix);
    void createMesh(const std::string &objName);
};

// 셰이더 프로그램 (전역, main.cpp에서 초기화됨)
class ShaderProgram;
extern ShaderProgram* g_shaderProgram;

struct MatrixStack {
    std::vector<glm::mat4x4> stack;
    MatrixStack();
    void loadIdentity();
    void matMul(glm::mat4x4 m);
    glm::mat4x4 getTopMatrix();
    glm::mat4x4 getTopNormal();
    void translate(float x, float y, float z);
    void rotate(float angle, float x, float y, float z);
    void scale(float x, float y, float z);
    void matPush();
    void matPop();
};
