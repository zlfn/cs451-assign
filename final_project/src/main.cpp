#include "base.hpp"
#include "utils.hpp"
#include "shaders/shaders.hpp"

// 전역 변수
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

GLuint shaderProgram;
GLuint VAO;
GLuint VBO;
GLuint EBO;

// 셰이더 컴파일
GLuint createShaderProgram() {
    GLint success = false;
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();

    // 정점 셰이더 컴파일
    glShaderSource(vertexShader, 1, &shaders::OCEAN_VERT_SHADER, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success); // 오류 검사
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex Shader Compile Failure:\n" << infoLog << '\n';
    }

    // 프래그먼트 셰이더 컴파일
    glShaderSource(fragmentShader, 1, &shaders::OCEAN_FRAG_SHADER, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success); // 오류 검사
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment Shader Compile Failure:\n" << infoLog << '\n';
    }

    // 셰이더 프로그램 링크
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success); // 링크 오류 검사
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader Program Link Failure:\n" << infoLog << '\n';
    }

    // 컴파일 & 링크 후 개별 shader 객체는 삭제
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void init() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    shaderProgram = createShaderProgram();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, GRID_SIZE * GRID_SIZE * 4 * sizeof(float), nullptr,
                 GL_DYNAMIC_DRAW);

    // Vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Create and bind EBO with indices for a grid mesh
    std::vector<unsigned int> indices;
    indices.reserve((GRID_SIZE - 1) * (GRID_SIZE - 1) * 6);
    for (int i = 0; i < GRID_SIZE - 1; ++i) {
        for (int j = 0; j < GRID_SIZE - 1; ++j) {
            unsigned int v0 = i * GRID_SIZE + j;
            unsigned int v1 = i * GRID_SIZE + j + 1;
            unsigned int v2 = (i + 1) * GRID_SIZE + j;
            unsigned int v3 = (i + 1) * GRID_SIZE + j + 1;
            // Triangle 1
            indices.push_back(v0);
            indices.push_back(v2);
            indices.push_back(v1);
            // Triangle 2
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v3);
        }
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void display(GLFWwindow* window) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);

    // Draw wireframe mesh
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, (GRID_SIZE - 1) * (GRID_SIZE - 1) * 6, GL_UNSIGNED_INT, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Reset for other potential draws

    glBindVertexArray(0);
    glUseProgram(0);

    glfwSwapBuffers(window);
}

float timeScale = 0.5f;
float lastFrameTime = 0.0f;
// 정점 데이터를 담을 벡터 (전역으로 두거나 timer 내에서 매번 생성)
std::vector<float> vertices;

void updateWaves() {
    float currentTime = glfwGetTime() * timeScale;
    calcWaveField(currentTime);
    iFFT();

    // currentHeight[i][j].real이 (i, j)에서의 파도 높이

    // VBO 업데이트 로직
    vertices.clear();
    vertices.reserve(GRID_SIZE * GRID_SIZE * 4); // 메모리 재할당 방지

    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            // 그리드 좌표 (j, i)를 NDC 좌표 (-1 ~ 1)로 매핑
            float x = (j / (float)(GRID_SIZE - 1)) * 2.0f - 1.0f;
            float y = (i / (float)(GRID_SIZE - 1)) * 2.0f - 1.0f;
            float z = 0.0f; // 2D 시각화이므로 z=0
            float height = currentHeight[i][j].real();

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(height); // 높이 값을 추가 속성으로 전달
        }
    }

    // VBO 데이터 업데이트
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
    std::cout << "Freed All Resources." << '\n';
}

int main(int argc, char **argv) {
    // GLFW 초기화
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // OpenGL 4.6 Core Profile 설정
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

    glEnable(GL_DEPTH_TEST); // 3D 렌더링을 위한 깊이 테스트 활성화
    std::cout << "Using OpenGL " << glGetString(GL_VERSION) << '\n';

    // 콜백 함수 설정
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    // VSync 활성화 (60 FPS로 제한)
    glfwSwapInterval(1);

    init(); // 셰이더, VBO/VAO 생성
    initSpectra(); // spectrum 초기화

    lastFrameTime = glfwGetTime();

    // 메인 루프
    while (!glfwWindowShouldClose(window)) {
        double currentFrameTime = glfwGetTime();
        double deltaTime = currentFrameTime - lastFrameTime;

        updateWaves();
        display(window);
        glfwPollEvents();
    }

    // 정리
    cleanup();
    glfwTerminate();

    return 0;
}
