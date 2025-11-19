#include "base.hpp"
#include "utils.hpp"
#include <cmath>
#include "shaders/shaders.hpp"

glm::vec2 w = {1.0, 0.0}; // Wind direction

const float L_world = 32.0f; // 시뮬레이션 월드 물리적 크기. 여기서는 32m x 32m

// 전역 변수로 선언
std::complex<float> initHeight[GRID_SIZE][GRID_SIZE] = {};
std::complex<float> initHeightConju[GRID_SIZE][GRID_SIZE] = {};
std::complex<float> currentHeight[GRID_SIZE][GRID_SIZE] = {};

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
    const float A = 0.5; // amplitude
    const float Lsqu = 0.25; // L = V^2/g = 0.5
    const float lsqu = 0.00001; // l = 0.001

    float Ksqu = kx * kx + ky * ky;

    // 0으로 나누기 방지
    if (Ksqu < 0.000001f) {
        return 0.0f;
    }

    float lowFrequencyDamping = std::exp(-1.0f / (Ksqu * Lsqu));
    float highFrequencyDamping = std::exp(-Ksqu * lsqu);
    float windFactor = kx * w.x + ky * w.y;
    windFactor = windFactor * windFactor;

    return A * windFactor * lowFrequencyDamping * highFrequencyDamping / (Ksqu * Ksqu);
}

float dispersion(float kx, float ky) {
    const float g = 9.8f; // gravity constant
    float k = std::sqrt(kx * kx + ky * ky);
    return std::sqrt(g * k);
}

void initSpectra() {
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

void calcWaveField(float t) {
    for (unsigned i = 0; i < GRID_SIZE; i++) {
        for (unsigned j = 0; j < GRID_SIZE; j++) {
            float kx_idx = (i < GRID_SIZE / 2) ? (float)i : (float)i - (float)GRID_SIZE;
            float kz_idx = (j < GRID_SIZE / 2) ? (float)j : (float)j - (float)GRID_SIZE;

            float kx_phys = kx_idx * (2.0f * std::numbers::pi_v<float> / L_world);
            float kz_phys = kz_idx * (2.0f * std::numbers::pi_v<float> / L_world);

            float disp = dispersion(kx_phys, kz_phys); // k로 dispersion 계산

            std::complex<float> z(0.0f, disp * t);
            std::complex<float> ez = std::exp(z);
            std::complex<float> ezc = std::conj(ez);

            currentHeight[i][j] = initHeight[i][j] * ez + initHeightConju[i][j] * ezc;
        }
    }
}

GLuint gComputeProgramH = 0; // horizontal
GLuint gComputeProgramV = 0; // vertical

GLuint gBaseSSBO = 0; // initHeight (고정 스펙트럼)
GLuint gTempSSBO = 0; // 가로 패스 결과
GLuint gCurrSSBO = 0; // 세로 패스 결과 (최종 height)

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
    // 가로 / 세로 셰이더를 각각 컴파일 & 링크
    GLuint csh = compileShader(GL_COMPUTE_SHADER, shaders::IFFT_HORI_COMP_SHADER); // horizontal
    GLuint csv = compileShader(GL_COMPUTE_SHADER, shaders::IFFT_VERT_COMP_SHADER); // vertical

    {
        std::vector<GLuint> shaderList;
        shaderList.push_back(csh);
        gComputeProgramH = linkProgram(shaderList); // 가로 패스용 프로그램
    }
    {
        std::vector<GLuint> shaderList;
        shaderList.push_back(csv);
        gComputeProgramV = linkProgram(shaderList); // 세로 패스용 프로그램
    }

    // CPU 쪽 2D 그리드 -> 1D 배열로 변환
    std::vector<Complex> dataGrid2D(GRID_SIZE * GRID_SIZE);

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            dataGrid2D[y * GRID_SIZE + x].re = initHeight[y][x].real();
            dataGrid2D[y * GRID_SIZE + x].im = initHeight[y][x].imag();
        }
    }

    // initHeight -> gBaseSSBO (고정 입력 스펙트럼)
    glGenBuffers(1, &gBaseSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gBaseSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataGrid2D.size() * sizeof(Complex), dataGrid2D.data(),
                 GL_STATIC_DRAW);

    // 가로 패스 결과용 temp 버퍼
    glGenBuffers(1, &gTempSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gTempSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataGrid2D.size() * sizeof(Complex), nullptr,
                 GL_DYNAMIC_COPY);

    // 세로 패스 최종 결과 버퍼
    glGenBuffers(1, &gCurrSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gCurrSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataGrid2D.size() * sizeof(Complex), nullptr,
                 GL_DYNAMIC_COPY);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void runIFFTCompute(float time) {
    // CPU 스펙트럼 H(k, t) 업데이트
    calcWaveField(time); // currentHeight[i][j] = H(k, t)

    // currentHeight를 gBaseSSBO에 업로드 (H(k, t) -> SSBO)
    static std::vector<Complex> dataGrid2D(GRID_SIZE * GRID_SIZE);

    for (int y = 0; y < GRID_SIZE; ++y) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            int idx = y * GRID_SIZE + x;
            dataGrid2D[idx].re = currentHeight[y][x].real();
            dataGrid2D[idx].im = currentHeight[y][x].imag();
        }
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gBaseSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, dataGrid2D.size() * sizeof(Complex),
                    dataGrid2D.data());

    // 가로 방향 iFFT
    glUseProgram(gComputeProgramH);

    GLint gs_loc_h = glGetUniformLocation(gComputeProgramH, "uGridSize");
    glUniform1i(gs_loc_h, GRID_SIZE);
    // uCurrTime은 이제 필요 없음 (calcWaveField에서 이미 반영했으니까)

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gBaseSSBO); // 입력: H(k, t)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gTempSSBO); // 출력: 가로 iFFT

    glDispatchCompute(GRID_SIZE, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 세로 방향 iFFT
    glUseProgram(gComputeProgramV);

    GLint gs_loc_v = glGetUniformLocation(gComputeProgramV, "uGridSize");
    glUniform1i(gs_loc_v, GRID_SIZE);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gTempSSBO); // 입력: 가로 iFFT 결과
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gCurrSSBO); // 출력: 최종 2D iFFT

    glDispatchCompute(GRID_SIZE, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

///////////////////////////////////////
