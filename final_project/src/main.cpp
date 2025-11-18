#include "base.hpp"
#include "utils.hpp"

// 전역 변수
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

GLuint shaderProgram;
GLuint VAO;
GLuint VBO;
GLuint EBO;

// 셰이더 소스 수정
const char *pointVertexShaderSrc = R"(
#version 450 core

struct Complex {
    float re;
    float im;
};

layout(std430, binding = 1) readonly buffer HeightData {
    Complex height[];
};

uniform int uGridSize;
uniform float uHeightScale;

out float vHeight;

void main() {
    int N   = uGridSize;
    int idx = gl_VertexID;   // 0 .. N*N-1

    int x = idx % N;
    int y = idx / N;

    // [-1, 1] 범위로 정규화한 평면 좌표 (XY plane)
    float fx = (float(x) / float(N - 1)) * 2.0 - 1.0;
    float fy = (float(y) / float(N - 1)) * 2.0 - 1.0;

    float h = height[idx].re * uHeightScale;

    // 예전이랑 동일: 평면은 XY, 높이는 Z
    vec3 pos = vec3(fx, fy, h);

    float angle = -1.2; // ~70도
    mat4 rotation = mat4(
        1.0, 0.0,        0.0,        0.0,
        0.0, cos(angle), -sin(angle), 0.0,
        0.0, sin(angle),  cos(angle), 0.0,
        0.0, 0.0,        0.0,        1.0
    );

    gl_Position = rotation * vec4(pos, 1.0);
    gl_PointSize = 3.0;

    vHeight = h;
}
)";

const char *pointFragmentShaderSrc = R"(
#version 450 core

in float vHeight;
out vec4 FragColor;

// 높이 값을 컬러로 매핑
vec3 heightToColor(float t) {
    float value = (t * 0.5) + 0.5; // -1~1 -> 0~1 (대충)

    vec3 low  = vec3(0.0, 0.0, 1.0);  // 파랑
    vec3 mid  = vec3(0.0, 1.0, 0.0);  // 초록
    vec3 high = vec3(1.0, 0.0, 0.0);  // 빨강

    vec3 color = mix(low, mid, value * 2.0);
    if (value > 0.5) {
        color = mix(mid, high, (value - 0.5) * 2.0);
    }
    return color;
}

void main() {
    FragColor = vec4(heightToColor(vHeight * 100.0), 1.0);
}
)";

// 셰이더 컴파일
GLuint createPointShaderProgram() {
    GLint success = GL_FALSE;
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint prog = glCreateProgram();

    glShaderSource(vs, 1, &pointVertexShaderSrc, nullptr);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(vs, 512, nullptr, log);
        std::cerr << "Point Vertex Shader Compile Failure:\n" << log << '\n';
    }

    glShaderSource(fs, 1, &pointFragmentShaderSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(fs, 512, nullptr, log);
        std::cerr << "Point Fragment Shader Compile Failure:\n" << log << '\n';
    }

    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "Point Shader Program Link Failure:\n" << log << '\n';
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

float timeScale = 0.5f;
float lastFrameTime = 0.0f;
// 정점 데이터를 담을 벡터 (전역으로 두거나 timer 내에서 매번 생성)
std::vector<float> vertices;

GLuint gPointVAO = 0;
GLuint gPointProgram = 0;

void initPointDraw() {
    glGenVertexArrays(1, &gPointVAO);
    glBindVertexArray(gPointVAO);
    gPointProgram = createPointShaderProgram();
    glBindVertexArray(0);
}

void drawIFFTPoints() {
    glUseProgram(gPointProgram);
    glBindVertexArray(gPointVAO);

    // SSBO 바인딩 (vertex shader에서 binding = 1)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gCurrSSBO);

    GLint locGrid = glGetUniformLocation(gPointProgram, "uGridSize");
    GLint locScale = glGetUniformLocation(gPointProgram, "uHeightScale");
    glUniform1i(locGrid, GRID_SIZE);
    glUniform1f(locScale, 1.0f);

    glDrawArrays(GL_POINTS, 0, GRID_SIZE * GRID_SIZE);

    glBindVertexArray(0);
    glUseProgram(0);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// 리소스 초기화
void cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glDeleteVertexArrays(1, &gPointVAO);
    glDeleteProgram(gPointProgram);

    glDeleteBuffers(1, &gBaseSSBO);
    glDeleteBuffers(1, &gTempSSBO);
    glDeleteBuffers(1, &gCurrSSBO);
    glDeleteProgram(gComputeProgramH);
    glDeleteProgram(gComputeProgramV);

    std::cout << "Freed All Resources." << '\n';
}

int main(int argc, char **argv) {
    // GLFW 초기화
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // OpenGL 4.5 Core Profile 설정
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 윈도우 생성
    GLFWwindow* window = glfwCreateWindow(800, 600, "CSED451 Final Project", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // GLEW 초기화
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW Initialization Failure: " << glewGetErrorString(err) << '\n';
        glfwTerminate();
        return -1;
    }
    
    glEnable(GL_DEPTH_TEST);
    std::cout << "Using OpenGL " << glGetString(GL_VERSION) << '\n';

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSwapInterval(1);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    initSpectra();       // 스펙트럼 초기화 (기존 그대로)
    initComputeShader(); // <-- compute 셰이더 & gBaseSSBO/gTempSSBO/gCurrSSBO 준비
    initPointDraw();     // <-- point 렌더링 셰이더 + VAO 준비

    lastFrameTime = glfwGetTime();

    // 메인 루프
    while (!glfwWindowShouldClose(window)) {
        double currentFrameTime = glfwGetTime();
        double deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        float currentTime = (float) currentFrameTime * timeScale;

        // 1) GPU에서 iFFT 돌려서 gCurrSSBO 채우기
        runIFFTCompute(currentTime);

        // 2) 화면 클리어 + 렌더 + 버퍼 스왑
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawIFFTPoints();
        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    // 정리
    cleanup();
    glfwTerminate();

    return 0;
}
