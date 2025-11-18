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
              << " objects from " << FILE_PATH << '\n';
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

//////////////// GPU ////////////////

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

std::string ifftCSH = R"(
#version 450

// cpp의 struct와 동일
struct Complex {
    float re;
    float im;
};

uniform int uGridSize;    // 반드시 32라고 가정 (shared[32], local_size_x = 32와 일치)
uniform float uCurrTime;

// 워크그룹당 32개의 쓰레드 = 한 행(row) 전체를 담당
layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

// SSBO 데이터
layout(std430, binding = 0) readonly buffer BaseData { Complex base[]; };
layout(std430, binding = 1) writeonly buffer CurrData { Complex curr[]; };

// 한 행을 워크그룹 내 공유 메모리에 올려서 계산
shared Complex rowShared[32];

// 복소수 연산 헬퍼
Complex c_add(Complex a, Complex b) {
    Complex r;
    r.re = a.re + b.re;
    r.im = a.im + b.im;
    return r;
}

Complex c_sub(Complex a, Complex b) {
    Complex r;
    r.re = a.re - b.re;
    r.im = a.im - b.im;
    return r;
}

Complex c_mul(Complex a, Complex b) {
    Complex r;
    r.re = a.re * b.re - a.im * b.im;
    r.im = a.re * b.im + a.im * b.re;
    return r;
}

// x의 하위 log2N 비트를 뒤집는 bit-reversal
uint bit_reverse(uint x, uint log2N) {
    uint r = 0u;
    for (uint i = 0u; i < log2N; ++i) {
        r = (r << 1u) | (x & 1u);
        x >>= 1u;
    }
    return r;
}

void main() {
    // 한 워크그룹 = 한 행(row)
    uint row = gl_WorkGroupID.x;       // 0 .. uGridSize-1
    uint col = gl_LocalInvocationID.x; // 0 .. 31

    uint N = uint(uGridSize);          // 그리드 크기 (32로 가정)
    if (col >= N) {
        return;
    }

    // 2D -> 1D 인덱스 변환
    uint idx1D = row * N + col;

    // global -> shared 로 한 행 전체 로딩
    rowShared[col] = base[idx1D];
    barrier(); // 워크그룹 내 모든 쓰레드 동기화

    // 여기부터 1D iFFT (radix-2 DIT)
    // N=32라면 log2N=5
    const uint LOG2N = 5u; // uGridSize가 항상 32라고 가정

    uint tid = col;

    // bit-reversal
    {
        uint j = bit_reverse(tid, LOG2N);
        // j < N 조건도 추가해서 범위 보호
        if (tid < j && j < N) {
            Complex tmp = rowShared[tid];
            rowShared[tid] = rowShared[j];
            rowShared[j] = tmp;
        }
        barrier();
    }

    // 스테이지별 버터플라이 루프
    for (uint s = 1u; s <= LOG2N; ++s) {
        uint m    = 1u << s;   // 이번 스테이지에서 묶는 블록 크기 (2,4,8,...,N)
        uint mh = m >> 1u;   // 블록 절반 길이

        uint blockIndex  = tid / m;  // 몇 번째 블록인지
        uint insideIndex = tid % m;  // 블록 안에서 몇 번째인지 (0..m-1)

        if (insideIndex < mh) {
            uint i0 = blockIndex * m + insideIndex;
            uint i1 = i0 + mh;

            // 혹시 N보다 크지 않도록 방어
            if (i1 < N) {
                Complex u = rowShared[i0];
                Complex v = rowShared[i1];

                // iFFT에서는 twiddle = exp(+2π i k / m)
                float k  = float(insideIndex);
                float mm = float(m);
                float angle = 2.0 * 3.14159265358979323846 * k / mm;

                Complex w;
                w.re = cos(angle);
                w.im = sin(angle);

                Complex t = c_mul(v, w);

                rowShared[i0] = c_add(u, t);
                rowShared[i1] = c_sub(u, t);
            }
        }

        // 스테이지 끝날 때마다 동기화
        barrier();
    }

    // iFFT 스케일링 (1/N 곱하기)
    Complex val = rowShared[tid];
    float invN = 1.0 / float(N);
    val.re *= invN;
    val.im *= invN;
    rowShared[tid] = val;

    barrier();

    // 결과를 shared에서 global로 다시 저장
    curr[idx1D] = rowShared[col];
}
)";

std::string ifftCSV = R"(
#version 450

// cpp의 struct와 동일
struct Complex {
    float re;
    float im;
};

uniform int uGridSize; // 반드시 32라고 가정

// 워크그룹당 32개의 쓰레드 = 한 열(column) 전체를 담당
layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

// SSBO 데이터
// 가로 방향 iFFT 결과를 input으로 사용한다고 가정
layout(std430, binding = 0) readonly buffer InData  { Complex inData[];  };
layout(std430, binding = 1) writeonly buffer OutData { Complex outData[]; };

// 한 열을 워크그룹 내 공유 메모리에 올려서 계산
shared Complex colShared[32];

// 복소수 연산 헬퍼

Complex c_add(Complex a, Complex b) {
    Complex r;
    r.re = a.re + b.re;
    r.im = a.im + b.im;
    return r;
}

Complex c_sub(Complex a, Complex b) {
    Complex r;
    r.re = a.re - b.re;
    r.im = a.im - b.im;
    return r;
}

Complex c_mul(Complex a, Complex b) {
    Complex r;
    r.re = a.re * b.re - a.im * b.im;
    r.im = a.re * b.im + a.im * b.re;
    return r;
}

// x의 하위 log2N 비트를 뒤집는 bit-reversal
uint bit_reverse(uint x, uint log2N) {
    uint r = 0u;
    for (uint i = 0u; i < log2N; ++i) {
        r = (r << 1u) | (x & 1u);
        x >>= 1u;
    }
    return r;
}

void main() {
    uint N = uint(uGridSize);          // 32라고 가정
    uint col = gl_WorkGroupID.x;       // 열 인덱스 0 .. N-1
    uint row = gl_LocalInvocationID.x; // 열 안에서 y 인덱스 0 .. 31

    if (row >= N) {
        return;
    }

    // 2D -> 1D 인덱스 변환 (row-major: idx = y * width + x)
    uint idx1D = row * N + col;

    // global -> shared 로 한 열 전체 로딩
    colShared[row] = inData[idx1D];
    barrier(); // 워크그룹 내 모든 쓰레드 동기화

    // 1D iFFT (radix-2 DIT
    const uint LOG2N = 5u; // N=32일 때 log2N=5
    uint tid = row;

    // bit-reversal
    {
        uint j = bit_reverse(tid, LOG2N);
        if (tid < j && j < N) {
            Complex tmp = colShared[tid];
            colShared[tid] = colShared[j];
            colShared[j] = tmp;
        }
        barrier();
    }

    // 2단계: 스테이지별 버터플라이 루프
    for (uint s = 1u; s <= LOG2N; ++s) {
        uint m    = 1u << s;   // 이번 스테이지에서 묶는 블록 크기 (2,4,8,...,N)
        uint mh = m >> 1u;   // 블록 절반 길이

        uint blockIndex  = tid / m;  // 몇 번째 블록인지
        uint insideIndex = tid % m;  // 블록 안에서 몇 번째인지 (0..m-1)

        if (insideIndex < mh) {
            uint i0 = blockIndex * m + insideIndex;
            uint i1 = i0 + mh;

            if (i1 < N) {
                Complex u = colShared[i0];
                Complex v = colShared[i1];

                // iFFT: twiddle = exp(+2π i k / m)
                float k  = float(insideIndex);
                float mm = float(m);
                float angle = 2.0 * 3.14159265358979323846 * k / mm;

                Complex w;
                w.re = cos(angle);
                w.im = sin(angle);

                Complex t = c_mul(v, w);

                colShared[i0] = c_add(u, t);
                colShared[i1] = c_sub(u, t);
            }
        }

        barrier();
    }

    // iFFT 스케일링 (1/N 곱하기)
    Complex val = colShared[tid];
    float invN = 1.0 / float(N);
    val.re *= invN;
    val.im *= invN;
    colShared[tid] = val;

    barrier();

    // 4) 결과를 shared에서 global로 다시 저장
    outData[idx1D] = colShared[row];
}
)";

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
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        std::cerr << "[Shader Compile Error]\n" << log << std::endl;
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
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLen;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetProgramInfoLog(program, logLen, nullptr, log.data());
        std::cerr << "[Program Link Error]\n" << log << std::endl;
        throw std::runtime_error("Program linking failed");
    }

    // 셰이더는 프로그램에 들어갔으면 지워도 됨
    for (auto s : shaders)
        glDeleteShader(s);

    return program;
}

void initComputeShader() {
    // 가로 / 세로 셰이더를 각각 컴파일 & 링크
    GLuint csh = compileShader(GL_COMPUTE_SHADER, ifftCSH); // horizontal
    GLuint csv = compileShader(GL_COMPUTE_SHADER, ifftCSV); // vertical

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
