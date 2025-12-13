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
const float HEIGHT_SCALE = 2.0f;
const float TERRAIN_HEIGHT_SCALE = 1.0f;
const float L_world = 64.0f; // 시뮬레이션 월드 물리적 크기. 단위는 meter
const glm::vec2 w = glm::normalize(glm::vec2(0.5, 0.5));    // Wind direction, MUST be normalized
const float lambda = 5.0f;

void initSpectrum();

// tessendorf waves
extern GLuint gWaveSpectrumCS;      // current wave spectrum calculation
extern GLuint gHorizontalIFFTCS;    // horizontal iFFT
extern GLuint gVerticalIFFTCS;      // vertical iFFT

extern GLuint gInitSpectrumSSBO;    // initial height
extern GLuint gInitSpectrumConjSSBO;// initial height conjugation
extern GLuint gCurrSpectrumSSBO;    // current height
extern GLuint gIFFTTempSSBO;        // iFFT temp
extern GLuint gCurrTessenHeightSSBO;// final tessendorf height
extern GLuint gCurrDXSSBO;          // x-displacement
extern GLuint gCurrDYSSBO;          // y-displacement

// SWE
extern GLuint gPDESolverCS;         // SWE solver

extern GLuint gTerrainHeightSSBO; // terrain height
extern GLuint gSpongeMaskSSBO;    // boundary attenuation mask. 1 for boundary
extern GLuint gBlendMaskSSBO;     // blend mask. 1 for tessendorf
extern GLuint gFinalZSSBO;        // final rendering map

// buffer A
extern GLuint gHeightASSBO;
extern GLuint gVelUASSBO;
extern GLuint gVelVASSBO;

// buffer B
extern GLuint gHeightBSSBO;
extern GLuint gVelUBSSBO;
extern GLuint gVelVBSSBO;

struct Complex {
    float re;
    float im;
};

void initComputeShader();
GLuint compileShader(GLenum type, const std::string &src);
GLuint linkProgram(const std::vector<GLuint> &shaders);
void calcPipeline(float time);

// ocean floor
extern GLuint gOceanFloorVAO;
extern GLuint gOceanFloorVBO;
extern GLuint gOceanFloorEBO;
extern GLuint gOceanFloorTexture;

void initOceanFloor();
void cleanupOceanFloor();

// bubble texture
extern GLuint gBubbleTexture;
GLuint loadTexture(const std::string &path);
void initBubbleTexture();
void cleanupBubbleTexture();

// ocean normal map
extern GLuint gOceanNormalTexture;
void initOceanNormalTexture();
void cleanupOceanNormalTexture();

// island normal map
extern GLuint gIslandNormalTexture;
void initIslandNormalTexture();
void cleanupIslandNormalTexture();

// Skybox
extern GLuint gSkyboxVAO;
extern GLuint gSkyboxVBO;
extern GLuint gSkyboxTexture;
extern GLuint gSkyboxProgram;

void initSkybox();
void drawSkybox(const glm::mat4 &view, const glm::mat4 &projection);
void cleanupSkybox();
GLuint loadCubemap(const std::vector<std::string>& faces);
