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

#define GRID_SIZE 32

extern std::complex<float> currentHeight[GRID_SIZE][GRID_SIZE];
void initSpectra();

// GPU
extern GLuint gComputeProgramH;
extern GLuint gComputeProgramV;

extern GLuint gBaseSSBO;
extern GLuint gTempSSBO;
extern GLuint gCurrSSBO;

struct Complex {
    float re;
    float im;
};

void initComputeShader();
void runIFFTCompute(float time);
GLuint compileShader(GLenum type, const std::string &src);
GLuint linkProgram(const std::vector<GLuint> &shaders);
