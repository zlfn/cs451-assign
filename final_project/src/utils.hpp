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
const unsigned int GRID_SIZE = 128; // IFFT resolution
const unsigned int RENDER_GRID_SIZE = GRID_SIZE * SMOOTH_FACTOR;  // High-res rendering mesh
const float HEIGHT_SCALE = 3.5f;
const float L_world = 32.0f; // 시뮬레이션 월드 물리적 크기. 여기서는 32m x 32m
const glm::vec2 w = {0.5, 0.5};    // Wind direction
const float lambda = 0.1f;

void initSpectrum();

// GPU
extern GLuint gWaveSpectrumCS;      // current wave spectrum calculation
extern GLuint gHorizontalIFFTCS;    // horizontal iFFT
extern GLuint gVerticalIFFTCS;      // vertical iFFT

extern GLuint gInitSpectrumSSBO;    // initial height
extern GLuint gInitSpectrumConjSSBO;// initial height conjugation
extern GLuint gCurrSpectrumSSBO;    // current height
extern GLuint gIFFTTempSSBO;        // iFFT temp
extern GLuint gCurrHeightSSBO;      // 최종 height
extern GLuint gCurrDXSSBO;          // D_x
extern GLuint gCurrDYSSBO;          // D_y

struct Complex {
    float re;
    float im;
};

void initComputeShader();
GLuint compileShader(GLenum type, const std::string &src);
GLuint linkProgram(const std::vector<GLuint> &shaders);
void calcPipeline(float time);

// Skybox
extern GLuint gSkyboxVAO;
extern GLuint gSkyboxVBO;
extern GLuint gSkyboxTexture;
extern GLuint gSkyboxProgram;

void initSkybox();
void drawSkybox(const glm::mat4& view, const glm::mat4& projection);
void cleanupSkybox();
GLuint loadCubemap(const std::vector<std::string>& faces);
