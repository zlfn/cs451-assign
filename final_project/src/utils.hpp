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

// Rendering Mesh resolution factor
// Higher Smooth factor provides more smooth wave. But requires higher GPU spec.
const unsigned int SMOOTH_FACTOR = 1;

const unsigned int GRID_SIZE = 128;  // IFFT resolution
const unsigned int RENDER_GRID_SIZE = GRID_SIZE * SMOOTH_FACTOR;  // High-res rendering mesh
const float HEIGHT_SCALE = 3.5f;

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

// Skybox
extern GLuint gSkyboxVAO;
extern GLuint gSkyboxVBO;
extern GLuint gSkyboxTexture;
extern GLuint gSkyboxProgram;

void initSkybox();
void drawSkybox(const glm::mat4& view, const glm::mat4& projection);
void cleanupSkybox();
GLuint loadCubemap(const std::vector<std::string>& faces);
