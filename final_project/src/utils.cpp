#include "base.hpp"
#include "utils.hpp"
#include <cmath>

ThreeDObj::ThreeDObj(const std::string &FILE_PATH, const glm::fvec3 &color)
    : objectColor(color) // 기본은 흰색
{
    try {
        getObjFile(FILE_PATH);
    } catch (const std::exception &e) {
        std::cerr << "Error loading object: " << e.what() << '\n';
    }
}

void ThreeDObj::getObjFile(const std::string &FILE_PATH) {
    std::ifstream file(FILE_PATH);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + FILE_PATH);
    }

    std::string currentObjName = "base";

    baseVertices.clear();
    objIndicesMap.clear();
    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "o") {
            Indices newIndices;
            ss >> currentObjName;
            objIndicesMap.insert({currentObjName, newIndices});
        } else if (prefix == "v") {
            glm::vec3 vertex;
            ss >> vertex.x >> vertex.y >> vertex.z;
            baseVertices.push_back(vertex);
        } else if (prefix == "f" && currentObjName != "") {
            std::vector<unsigned int> faceIndices;
            std::string vertexToken;
            while (ss >> vertexToken) {
                std::stringstream tokenSS(vertexToken);
                std::string indexStr;
                std::getline(tokenSS, indexStr, '/');
                faceIndices.push_back(std::stoul(indexStr) - 1);
            }

            for (size_t i = 1; i < faceIndices.size() - 1; ++i) {
                objIndicesMap[currentObjName].push_back(faceIndices[0]);
                objIndicesMap[currentObjName].push_back(faceIndices[i]);
                objIndicesMap[currentObjName].push_back(faceIndices[i + 1]);
            }
        }
    }
    file.close();

    for (const auto &[key, value] : objIndicesMap) {
        Indices currentIndices = value;
        glm::vec3 centerPos = glm::vec3(0.0,0.0,0.0);
        for (const auto &vIndex : currentIndices) {
            centerPos += baseVertices[vIndex];
        }
        centerPos /= currentIndices.size();
        objCenterMap.insert({key, centerPos});
    }

    std::cout << "Loaded " << baseVertices.size() << " vertices, " << (objIndicesMap.size())
              << " objects from " << FILE_PATH << std::endl;
}

void ThreeDObj::setColor(const glm::vec3 &color) { objectColor = color; }

void ThreeDObj::draw(const std::string objName) {
    if (baseVertices.empty() || objIndicesMap.empty()) {
        return;
    }

    Indices indices;
    try {
        indices = objIndicesMap[objName];
    } catch (const std::out_of_range &oor) {
        std::cerr << "Error: there is no object named " << objName << std::endl;
    }

    glLineWidth(1.0f);
    glColor3f(objectColor.x, objectColor.y, objectColor.z);

    // 삼각형 와이어프레임 그리기
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_TRIANGLES);
    {
        for (unsigned int index : indices) {
            const glm::vec3 &vertex = baseVertices[index];
            glVertex3f(vertex.x, vertex.y, vertex.z);
        }
    }
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// a, b를 포함하는 범위에서 랜덤 정수 반환
int getRandomRange(int a, int b) {
    static std::uniform_int_distribution<int> distInt(a, b);
    return static_cast<int>(dist(gen));
}

///////////// CPU VERSION /////////////

vec2 w = {1.0, 0.0}; // Wind direction

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

void iFFT_1D_inplace(std::complex<float> data[GRID_SIZE]) {
    // Bit-reversal permutation
    for (unsigned int i = 0; i < GRID_SIZE; i++) {
        unsigned int rev = 0;
        unsigned int temp_i = i;
        int num_bits = static_cast<int>(log2(GRID_SIZE));
        for (int j = 0; j < num_bits; j++) {
            rev = (rev << 1) | (temp_i & 1);
            temp_i >>= 1;
        }
        if (rev > i) {
            std::swap(data[i], data[rev]);
        }
    }

    // Cooley-Tukey FFT algorithm (Radix-2)
    for (int len = 2; len <= GRID_SIZE; len <<= 1) {
        int m = len / 2;
        std::complex<float> W_len =
            std::exp(std::complex<float>(0.0, 2.0 * std::numbers::pi_v<float> / len));
        for (int k = 0; k < GRID_SIZE; k += len) {
            std::complex<float> W = 1.0;
            for (int j = 0; j < m; j++) {
                std::complex<float> T = W * data[k + j + m];
                std::complex<float> U = data[k + j];

                data[k + j] = U + T;
                data[k + j + m] = U - T;

                W = W * W_len;
            }
        }
    }
}

void iFFT() {
    // 가로줄 iFFT
    for (unsigned i = 0; i < GRID_SIZE; i++) {
        iFFT_1D_inplace(currentHeight[i]);
    }

    // 세로줄 iFFT
    std::complex<float> temp_col[GRID_SIZE];
    for (unsigned j = 0; j < GRID_SIZE; j++) { // j번째 세로줄
        for (unsigned i = 0; i < GRID_SIZE; i++) {
            temp_col[i] = currentHeight[i][j];
        }

        iFFT_1D_inplace(temp_col);

        for (unsigned i = 0; i < GRID_SIZE; i++) {
            currentHeight[i][j] = temp_col[i];
        }
    }

    // 정규화
    float scale = 1.0f / (float)(GRID_SIZE * GRID_SIZE);
    for (unsigned i = 0; i < GRID_SIZE; i++) {
        for (unsigned j = 0; j < GRID_SIZE; j++) {
            currentHeight[i][j] *= scale;
        }
    }
}

/*
int waveMain() {
    initSpectra();

    float time = 0.0f;
    while (true) {
        time += 0.016f; // 60FPS
        calcWaveField(time);

        iFFT();
        // currentHeight[i][j].real이 (i, j)에서의 파도 높이
        std::cout << currentHeight[0][0] << std::endl;
    }

    return 0;
}
*/

///////////////////////////////////////
