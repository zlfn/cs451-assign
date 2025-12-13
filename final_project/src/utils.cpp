#include "base.hpp"
#include "utils.hpp"
#include <cmath>
#include "shaders/shaders.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// 전역 변수로 선언
std::complex<float> initHeight[GRID_SIZE][GRID_SIZE] = {};
std::complex<float> initHeightConju[GRID_SIZE][GRID_SIZE] = {};

GLuint gWaveSpectrumCS = 0;    // current wave spectrum calculation
GLuint gHorizontalIFFTCS = 0;  // horizontal iFFT
GLuint gVerticalIFFTCS = 0;   // vertical iFFT

GLuint gInitSpectrumSSBO = 0;     // initial height
GLuint gInitSpectrumConjSSBO = 0; // initial height conjugation
GLuint gCurrSpectrumSSBO = 0;     // current height
GLuint gIFFTTempSSBO = 0;       // iFFT temp
GLuint gCurrHeightSSBO = 0;           // 최종 height
GLuint gCurrDXSSBO = 0;         // D_x
GLuint gCurrDYSSBO = 0;         // D_y

///////////////// initiate spectrum /////////////////
std::complex<float> randGaussianComplex() {
    static std::mt19937 gen(std::random_device{}());
    // (0.0, 1.0] 범위. 0을 피하여 log(0) 방지
    static std::uniform_real_distribution<float> dist(std::nextafter(0.0f, 1.0f), 1.0f);

    float u1 = dist(gen);
    float u2 = dist(gen);

    // Box-Muller 변환
    float magnitude = std::sqrt(-2.0f * std::log(u1));
    float z0 = magnitude * std::cos(2.0f * std::numbers::pi_v<float> * u2);
    float z1 = magnitude * std::sin(2.0f * std::numbers::pi_v<float> * u2);

    return std::complex<float>(z0, z1);
}

float phillipsSpectrum(float kx, float ky) {
    const float A = 1.0; // amplitude
    const float Lsqu = 1600.0; // L = V^2/g = 40.0
    const float lsqu = 0.01; // l = 0.1

    float Ksqu = kx * kx + ky * ky;

    // 0으로 나누기 방지
    if (Ksqu < 0.000001f) {
        return 0.0f;
    }

    float dot_kw = kx * w.x + ky * w.y;
    float windFactor = dot_kw * dot_kw / Ksqu; // This is now |k_hat . w_hat|^2

    float lowFrequencyDamping = std::exp(-1.0f / (Ksqu * Lsqu));
    float highFrequencyDamping = std::exp(-Ksqu * lsqu);

    return A * windFactor * lowFrequencyDamping * highFrequencyDamping / (Ksqu * Ksqu);
}

void initSpectrum() {
    const float normFactor = 1.0f / std::sqrt(2.0f);

    for (unsigned i = 0; i < GRID_SIZE; i++) {
        for (unsigned j = 0; j < GRID_SIZE; j++) {
            float kx_idx = (i < GRID_SIZE / 2) ? (float)i : (float)i - (float)GRID_SIZE; // 파수
            float kz_idx = (j < GRID_SIZE / 2) ? (float)j : (float)j - (float)GRID_SIZE;

            float kx_phys = kx_idx * (2.0f * std::numbers::pi_v<float> / L_world); // 물리적 파수
            float kz_phys = kz_idx * (2.0f * std::numbers::pi_v<float> / L_world);

            float Ph = phillipsSpectrum(kx_phys, kz_phys); // phillips 스펙트럼
            initHeight[i][j] = normFactor * std::sqrt(Ph) * randGaussianComplex();
        }
    }

    // 켤레 대칭 계산
    for (unsigned i = 0; i < GRID_SIZE; i++) {
        for (unsigned j = 0; j < GRID_SIZE; j++) {
            int neg_i = (GRID_SIZE - i) % GRID_SIZE;
            int neg_j = (GRID_SIZE - j) % GRID_SIZE;

            initHeightConju[i][j] = std::conj(initHeight[neg_i][neg_j]);
        }
    }
}

///////////////// Shader /////////////////

GLuint compileShader(GLenum type, const std::string &src) {
    GLuint shader = glCreateShader(type);

    const char *csrc = src.c_str();
    glShaderSource(shader, 1, &csrc, nullptr);
    glCompileShader(shader);

    // 에러 체크
    GLint success = false;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        std::cerr << "[Shader Compile Error]\n" << log << '\n';
        throw std::runtime_error("Shader compilation failed");
    }

    return shader;
}

GLuint linkProgram(const std::vector<GLuint> &shaders) {
    GLuint program = glCreateProgram();
    for (auto s : shaders) {
        glAttachShader(program, s);
    }

    glLinkProgram(program);

    // 에러 체크
    GLint success = false;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetProgramInfoLog(program, logLen, nullptr, log.data());
        std::cerr << "[Program Link Error]\n" << log << '\n';
        throw std::runtime_error("Program linking failed");
    }

    // 셰이더는 프로그램에 들어갔으면 지워도 됨
    for (auto s : shaders)
        glDeleteShader(s);

    return program;
}

void initComputeShader() {
    GLuint ifftHorCS = compileShader(GL_COMPUTE_SHADER, shaders::IFFT_HORI_COMP_SHADER);
    GLuint ifftVerCS = compileShader(GL_COMPUTE_SHADER, shaders::IFFT_VERT_COMP_SHADER);
    GLuint waveSpeCS = compileShader(GL_COMPUTE_SHADER, shaders::WAVE_SPECTRUM_COMP_SHADER);

    {
        std::vector<GLuint> shaderList;
        shaderList.push_back(ifftHorCS);
        gHorizontalIFFTCS = linkProgram(shaderList);
    }
    {
        std::vector<GLuint> shaderList;
        shaderList.push_back(ifftVerCS);
        gVerticalIFFTCS = linkProgram(shaderList);
    }
    {
        std::vector<GLuint> shaderList;
        shaderList.push_back(waveSpeCS);
        gWaveSpectrumCS = linkProgram(shaderList);
    }

    // CPU 쪽 2D 그리드 -> 1D 배열로 변환
    std::vector<Complex> dataGrid2D(GRID_SIZE * GRID_SIZE);

    // gInitHeightSSBO
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            dataGrid2D[y * GRID_SIZE + x].re = initHeight[y][x].real();
            dataGrid2D[y * GRID_SIZE + x].im = initHeight[y][x].imag();
        }
    }
    glGenBuffers(1, &gInitSpectrumSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gInitSpectrumSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataGrid2D.size() * sizeof(Complex), dataGrid2D.data(),
                 GL_STATIC_DRAW);

    // gInitHeightConjSSBO
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            dataGrid2D[y * GRID_SIZE + x].re = initHeightConju[y][x].real();
            dataGrid2D[y * GRID_SIZE + x].im = initHeightConju[y][x].imag();
        }
    }
    glGenBuffers(1, &gInitSpectrumConjSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gInitSpectrumConjSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataGrid2D.size() * sizeof(Complex), dataGrid2D.data(),
                 GL_STATIC_DRAW);

    // gIFFTTempSSBO, 
    glGenBuffers(1, &gCurrSpectrumSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gCurrSpectrumSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataGrid2D.size() * sizeof(Complex), nullptr,
                 GL_DYNAMIC_COPY);
    glGenBuffers(1, &gIFFTTempSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gIFFTTempSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataGrid2D.size() * sizeof(Complex), nullptr,
                 GL_DYNAMIC_COPY);
    glGenBuffers(1, &gCurrHeightSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gCurrHeightSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataGrid2D.size() * sizeof(Complex), nullptr,
                 GL_DYNAMIC_COPY);
    glGenBuffers(1, &gCurrDXSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gCurrDXSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataGrid2D.size() * sizeof(Complex), nullptr,
                 GL_DYNAMIC_COPY);
    glGenBuffers(1, &gCurrDYSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gCurrDYSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataGrid2D.size() * sizeof(Complex), nullptr,
                 GL_DYNAMIC_COPY);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void calcCurrentSpectum(float time) {
    glUseProgram(gWaveSpectrumCS);

    // uniforms
    GLint locTime = glGetUniformLocation(gWaveSpectrumCS, "uTime");
    GLint locLWorld = glGetUniformLocation(gWaveSpectrumCS, "uLWorld");
    GLint locGridSize = glGetUniformLocation(gWaveSpectrumCS, "uGridSize");

    glUniform1f(locTime, time);
    glUniform1f(locLWorld, L_world);
    glUniform1i(locGridSize, GRID_SIZE);

    // SSBO 바인딩
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gInitSpectrumSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gInitSpectrumConjSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, gCurrSpectrumSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, gCurrDXSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, gCurrDYSSBO);

    // dispatch
    glDispatchCompute(GRID_SIZE, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void calcIFFT(GLuint baseSSBO, GLuint resultSSBO) {
    // 가로 방향 iFFT
    glUseProgram(gHorizontalIFFTCS);

    GLint gs_loc_h = glGetUniformLocation(gHorizontalIFFTCS, "uGridSize");
    glUniform1i(gs_loc_h, GRID_SIZE);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, baseSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gIFFTTempSSBO);

    glDispatchCompute(GRID_SIZE, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 세로 방향 iFFT
    glUseProgram(gVerticalIFFTCS);

    GLint gs_loc_v = glGetUniformLocation(gVerticalIFFTCS, "uGridSize");
    glUniform1i(gs_loc_v, GRID_SIZE);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gIFFTTempSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, resultSSBO);

    glDispatchCompute(GRID_SIZE, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void calcPipeline(float time) {
    calcCurrentSpectum(time);
    calcIFFT(gCurrSpectrumSSBO, gCurrHeightSSBO);
    calcIFFT(gCurrDXSSBO, gCurrDXSSBO);
    calcIFFT(gCurrDYSSBO, gCurrDYSSBO);
}

///////////////////////////////////////

GLuint gOceanFloorVAO = 0;
GLuint gOceanFloorVBO = 0;
GLuint gOceanFloorEBO = 0;
GLuint gOceanFloorTexture = 0;

GLuint loadTexture(const std::string &path) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;

    stbi_set_flip_vertically_on_load(true);

    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);

        // 텍스처 데이터 전송
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D); // 밉맵 자동 생성

        // 텍스처 파라미터 설정 (필터링 및 래핑)
        // 이걸 설정 안 하면 텍스처가 검은색으로 나올 수 있음
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "Loaded texture: " << path << std::endl;
    } else {
        std::cout << "Failed to load texture: " << path << std::endl;
        std::cout << "STB Reason: " << stbi_failure_reason() << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

float oceanFloorVertices[] = {
    -50.0f, 0.0f, -50.0f, 0.0f, 1.0f, 0.0f, 0.0f, 50.0f,
    50.0f, 0.0f, -50.0f, 0.0f, 1.0f, 0.0f, 50.0f, 50.0f,
    50.0f, 0.0f, 50.0f, 0.0f, 1.0f, 0.0f, 50.0f, 0.0f,
    -50.0f, 0.0f, 50.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};

unsigned int oceanFloorIndices[] = {
    0, 1, 2, // Triangle 1
    2, 3, 0  // Triangle 2
};

void initOceanFloor() {
    // 1. VAO, VBO, EBO 생성
    glGenVertexArrays(1, &gOceanFloorVAO);
    glGenBuffers(1, &gOceanFloorVBO);
    glGenBuffers(1, &gOceanFloorEBO); // 지형은 EBO를 사용하는 것이 효율적

    // 2. VAO 바인딩
    glBindVertexArray(gOceanFloorVAO);

    // 3. VBO 데이터 설정
    glBindBuffer(GL_ARRAY_BUFFER, gOceanFloorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(oceanFloorVertices), oceanFloorVertices, GL_STATIC_DRAW);

    // 4. EBO 데이터 설정
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gOceanFloorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(oceanFloorIndices), oceanFloorIndices,
                 GL_STATIC_DRAW);

    // 5. Vertex Attributes 설정 (Stride = 8 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));

    // texture load
    std::string floorTexturePath = "assets/floor.jpg";
    gOceanFloorTexture = loadTexture(floorTexturePath);

    // unbinding
    glBindVertexArray(0);
    std::cout << "Ocean Floor initialized\n";
}

void cleanupOceanFloor() {
    glDeleteVertexArrays(1, &gOceanFloorVAO);
    glDeleteBuffers(1, &gOceanFloorVBO);
    glDeleteBuffers(1, &gOceanFloorEBO);
    glDeleteTextures(1, &gOceanFloorTexture);
}

///////////////////////////////////////

GLuint gBubbleTexture = 0;

void initBubbleTexture() {
    std::string bubbleTexturePath = "assets/bubble.png";
    gBubbleTexture = loadTexture(bubbleTexturePath);
    std::cout << "Bubble texture initialized\n";
}

void cleanupBubbleTexture() {
    glDeleteTextures(1, &gBubbleTexture);
}

///////////////////////////////////////

GLuint gOceanNormalTexture = 0;

void initOceanNormalTexture() {
    std::string normalTexturePath = "assets/ocean_normal.jpg";
    gOceanNormalTexture = loadTexture(normalTexturePath);
    std::cout << "Ocean normal texture initialized\n";
}

void cleanupOceanNormalTexture() {
    glDeleteTextures(1, &gOceanNormalTexture);
}

///////////////////////////////////////

// Skybox global variables
GLuint gSkyboxVAO = 0;
GLuint gSkyboxVBO = 0;
GLuint gSkyboxTexture = 0;
GLuint gSkyboxProgram = 0;

// Skybox vertices (cube centered at origin)
float skyboxVertices[] = {
    // positions
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

GLuint loadCubemap(const std::vector<std::string>& faces) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    stbi_set_flip_vertically_on_load(false);

    for (unsigned int i = 0; i < faces.size(); i++) {
        int width, height, nrChannels;
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            else if (nrChannels == 4)
                format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
            std::cout << "Loaded cubemap face: " << faces[i] << " (" << width << "x" << height << ", " << nrChannels << " channels)\n";
        } else {
            std::cerr << "Cubemap texture failed to load at path: " << faces[i] << '\n';
            std::cerr << "STB Error: " << stbi_failure_reason() << '\n';
            stbi_image_free(data);
        }
    }

    // Generate mipmaps for IBL reflections
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // Use trilinear filtering with mipmaps
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void initSkybox() {
    // Create and compile skybox shader program
    GLuint vs = compileShader(GL_VERTEX_SHADER, shaders::SKYBOX_VERT_SHADER);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, shaders::SKYBOX_FRAG_SHADER);
    gSkyboxProgram = linkProgram({vs, fs});

    // Setup skybox VAO
    glGenVertexArrays(1, &gSkyboxVAO);
    glGenBuffers(1, &gSkyboxVBO);
    glBindVertexArray(gSkyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gSkyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // Load cubemap textures
    std::vector<std::string> faces = {
        "assets/skybox/right.png",   // +X
        "assets/skybox/left.png",    // -X
        "assets/skybox/up.png",      // +Y
        "assets/skybox/down.png",    // -Y
        "assets/skybox/front.png",   // +Z
        "assets/skybox/back.png"     // -Z
    };
    gSkyboxTexture = loadCubemap(faces);

    glBindVertexArray(0);
    std::cout << "Skybox initialized\n";
}

void drawSkybox(const glm::mat4& view, const glm::mat4& projection) {
    // Draw skybox last with depth function set to GL_LEQUAL
    glDepthFunc(GL_LEQUAL);
    glUseProgram(gSkyboxProgram);

    GLint viewLoc = glGetUniformLocation(gSkyboxProgram, "view");
    GLint projectionLoc = glGetUniformLocation(gSkyboxProgram, "projection");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);

    glBindVertexArray(gSkyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gSkyboxTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // Reset depth function to default
    glUseProgram(0);
}

void cleanupSkybox() {
    glDeleteVertexArrays(1, &gSkyboxVAO);
    glDeleteBuffers(1, &gSkyboxVBO);
    glDeleteTextures(1, &gSkyboxTexture);
    glDeleteProgram(gSkyboxProgram);
}
