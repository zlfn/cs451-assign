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
GLuint createPointShaderProgram() {
    GLint success = GL_FALSE;
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint prog = glCreateProgram();

    glShaderSource(vs, 1, &shaders::OCEAN_VERT_SHADER, nullptr);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(vs, 512, nullptr, log);
        std::cerr << "Point Vertex Shader Compile Failure:\n" << log << '\n';
    }

    glShaderSource(fs, 1, &shaders::OCEAN_FRAG_SHADER, nullptr);
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
GLuint gPointEBO = 0;
GLuint gPointProgram = 0;

// Camera state - shoreline view
glm::vec3 cameraPos = glm::vec3(0.0f, 0.2f, 1.5f);  // A bit higher and further back
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);  // Look at center of ocean
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 1.0f;

void initPointDraw() {
    glGenVertexArrays(1, &gPointVAO);
    glBindVertexArray(gPointVAO);

    // Create index buffer for high-resolution triangle mesh
    std::vector<GLuint> indices;
    for (int y = 0; y < RENDER_GRID_SIZE - 1; y++) {
        for (int x = 0; x < RENDER_GRID_SIZE - 1; x++) {
            int topLeft = y * RENDER_GRID_SIZE + x;
            int topRight = topLeft + 1;
            int bottomLeft = (y + 1) * RENDER_GRID_SIZE + x;
            int bottomRight = bottomLeft + 1;

            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    glGenBuffers(1, &gPointEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gPointEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    gPointProgram = createPointShaderProgram();
    glBindVertexArray(0);
}

void drawIFFTPoints(float currentTime, int windowWidth, int windowHeight) {
    glUseProgram(gPointProgram);
    glBindVertexArray(gPointVAO);

    // SSBO 바인딩 (vertex shader에서 binding = 1)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gCurrSSBO);

    // Grid and scale uniforms
    GLint locIFFTGrid = glGetUniformLocation(gPointProgram, "uIFFTGridSize");
    GLint locRenderGrid = glGetUniformLocation(gPointProgram, "uRenderGridSize");
    GLint locScale = glGetUniformLocation(gPointProgram, "uHeightScale");
    glUniform1i(locIFFTGrid, GRID_SIZE);
    glUniform1i(locRenderGrid, RENDER_GRID_SIZE);
    glUniform1f(locScale, HEIGHT_SCALE);

    // View matrix (looking at ocean from angle)
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    GLint locView = glGetUniformLocation(gPointProgram, "uView");
    glUniformMatrix4fv(locView, 1, GL_FALSE, &view[0][0]);

    // Projection matrix - narrower FOV to hide edges
    float aspect = (float)windowWidth / (float)windowHeight;
    glm::mat4 projection = glm::perspective(glm::radians(30.0f), aspect, 0.1f, 100.0f);
    GLint locProjection = glGetUniformLocation(gPointProgram, "uProjection");
    glUniformMatrix4fv(locProjection, 1, GL_FALSE, &projection[0][0]);

    // Draw filled triangles
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    int numIndices = (RENDER_GRID_SIZE - 1) * (RENDER_GRID_SIZE - 1) * 6;
    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);

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
    glDeleteBuffers(1, &gPointEBO);
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
    GLFWwindow* window = glfwCreateWindow(1600, 1200, "CSED451 Final Project", nullptr, nullptr);
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

    initSpectra();       // 스펙트럼 초기화
    initComputeShader(); // compute 셰이더 & gBaseSSBO/gTempSSBO/gCurrSSBO 준비
    initPointDraw();     // point 렌더링 셰이더 + VAO 준비

    lastFrameTime = glfwGetTime();

    // 메인 루프
    while (!glfwWindowShouldClose(window)) {
        double currentFrameTime = glfwGetTime();
        double deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        float currentTime = (float) currentFrameTime * timeScale;

        // GPU에서 iFFT 돌려서 gCurrSSBO 채우기
        runIFFTCompute(currentTime);

        // Get window size for aspect ratio
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        // 화면 클리어 + 렌더 + 버퍼 스왑
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawIFFTPoints(currentTime, windowWidth, windowHeight);
        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    // 정리
    cleanup();
    glfwTerminate();

    return 0;
}
