#include "base.hpp"
#include "utils.hpp"
#include "shaders/shaders.hpp"

// 전역 변수
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

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
        char log[128];
        glGetShaderInfoLog(vs, 128, nullptr, log);
        std::cerr << "Point Vertex Shader Compile Failure:\n" << log << '\n';
    }

    glShaderSource(fs, 1, &shaders::OCEAN_FRAG_SHADER, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[128];
        glGetShaderInfoLog(fs, 128, nullptr, log);
        std::cerr << "Point Fragment Shader Compile Failure:\n" << log << '\n';
    }

    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char log[128];
        glGetProgramInfoLog(prog, 128, nullptr, log);
        std::cerr << "Point Shader Program Link Failure:\n" << log << '\n';
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

float timeScale = 0.5f;
float lastFrameTime = 0.0f;

GLuint gPointVAO = 0;
GLuint gPointEBO = 0;
GLuint gPointProgram = 0;

// Camera state - shoreline view
glm::vec3 cameraPos = glm::vec3(0.0f, 0.4f, 2.5f);  // A bit higher and further back
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);  // Look at center of ocean
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 1.0f;

// Mouse state for light direction control
double mouseX = 0.5;  // Normalized [0, 1]
double mouseY = 0.5;  // Normalized [0, 1]

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

    // SSBO 바인딩 (vertex shader에서 binding = 0,1,2)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gCurrHeightSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gCurrDXSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, gCurrDYSSBO);

    // Grid and scale uniforms
    GLint locIFFTGrid = glGetUniformLocation(gPointProgram, "uIFFTGridSize");
    GLint locRenderGrid = glGetUniformLocation(gPointProgram, "uRenderGridSize");
    GLint locScale = glGetUniformLocation(gPointProgram, "uHeightScale");
    GLint locModel = glGetUniformLocation(gPointProgram, "uModel");
    glUniform1i(locIFFTGrid, GRID_SIZE);
    glUniform1i(locRenderGrid, RENDER_GRID_SIZE);
    glUniform1f(locScale, HEIGHT_SCALE);

    // Lambda (수평 변위 강도)
    GLint locLambda = glGetUniformLocation(gPointProgram, "uLambda");
    glUniform1f(locLambda, lambda); // 원하는 값으로 세팅 (0.0 ~ 2.0 정도로 튜닝)

    // View matrix (looking at ocean from angle)
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    GLint locView = glGetUniformLocation(gPointProgram, "uView");
    glUniformMatrix4fv(locView, 1, GL_FALSE, &view[0][0]);

    // Projection matrix
    float aspect = (float)windowWidth / (float)windowHeight;
    glm::mat4 projection = glm::perspective(glm::radians(75.0f), aspect, 0.1f, 100.0f);
    GLint locProjection = glGetUniformLocation(gPointProgram, "uProjection");
    glUniformMatrix4fv(locProjection, 1, GL_FALSE, &projection[0][0]);

    // PBR uniforms
    GLint locCameraPos = glGetUniformLocation(gPointProgram, "uCameraPos");
    glUniform3fv(locCameraPos, 1, &cameraPos[0]);

    // Light direction: from above camera view direction
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.0f, 1.3f, -0.7f));
    GLint locLightDir = glGetUniformLocation(gPointProgram, "uLightDir");
    glUniform3fv(locLightDir, 1, &lightDir[0]);

    // Light color (more yellow sunlight)
    glm::vec3 lightColor = glm::vec3(1.0f, 0.9f, 0.7f);
    GLint locLightColor = glGetUniformLocation(gPointProgram, "uLightColor");
    glUniform3fv(locLightColor, 1, &lightColor[0]);

    // Roughness (water is fairly smooth, but not perfect glass)
    GLint locRoughness = glGetUniformLocation(gPointProgram, "uRoughness");
    glUniform1f(locRoughness, 0.15f);

    // refraction strength
    GLint strengthLoc = glGetUniformLocation(gPointProgram, "uRefractionStrength");
    glUniform1f(strengthLoc, 0.02f);

    // Time for animated effects (foam, turbulence)
    GLint locTime = glGetUniformLocation(gPointProgram, "uTime");
    glUniform1f(locTime, currentTime);

    // Bind environment map for IBL reflections
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gSkyboxTexture);
    GLint locEnvMap = glGetUniformLocation(gPointProgram, "uEnvironmentMap");
    glUniform1i(locEnvMap, 0);

    // Bind ocean floor map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gOceanFloorTexture);
    GLint locOceMap = glGetUniformLocation(gPointProgram, "uRefractionTexture");
    glUniform1i(locOceMap, 1);

    // Bind bubble texture
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBubbleTexture);
    GLint locBubbleMap = glGetUniformLocation(gPointProgram, "uBubbleTexture");
    glUniform1i(locBubbleMap, 2);

    // Bind ocean normal map
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gOceanNormalTexture);
    GLint locNormalMap = glGetUniformLocation(gPointProgram, "uNormalMap");
    glUniform1i(locNormalMap, 3);

    // Draw filled triangles
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    int numIndices = (RENDER_GRID_SIZE - 1) * (RENDER_GRID_SIZE - 1) * 6;

    // Draw tiled ocean (5x5 grid centered around camera)
    const int tileRadius = 2;  // Creates 5x5 grid
    const float tileSize = 4.0f;  // Ocean mesh size is [-2, 2], so 4.0 total

    // Calculate camera forward vector for back-face culling
    glm::vec3 cameraForward = glm::normalize(cameraTarget - cameraPos);

    for (int tz = -tileRadius; tz <= tileRadius; tz++) {
        for (int tx = -tileRadius; tx <= tileRadius; tx++) {
            // Tile center in world space
            glm::vec3 tileCenter = glm::vec3(tx * tileSize, 0.0f, tz * tileSize);

            // Vector from camera to tile center
            glm::vec3 cameraToTile = tileCenter - cameraPos;

            // Skip tiles behind the camera (dot product < 0 means behind)
            // Use a small margin to avoid clipping tiles at the edge
            if (glm::dot(cameraToTile, cameraForward) < -tileSize) {
                continue;
            }

            // Create model matrix with tile offset
            glm::mat4 model = glm::translate(glm::mat4(1.0f), tileCenter);
            glUniformMatrix4fv(locModel, 1, GL_FALSE, &model[0][0]);

            // Draw this tile
            glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
        }
    }

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

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    // Normalize mouse coordinates to [0, 1]
    mouseX = xpos / width;
    mouseY = ypos / height;

    // Clamp to [0, 1]
    mouseX = glm::clamp(mouseX, 0.0, 1.0);
    mouseY = glm::clamp(mouseY, 0.0, 1.0);
}

// 리소스 초기화
void cleanup() {
    glDeleteVertexArrays(1, &gPointVAO);
    glDeleteBuffers(1, &gPointEBO);
    glDeleteProgram(gPointProgram);

    glDeleteBuffers(1, &gInitSpectrumSSBO);
    glDeleteBuffers(1, &gInitSpectrumConjSSBO);
    glDeleteBuffers(1, &gCurrSpectrumSSBO);
    glDeleteBuffers(1, &gIFFTTempSSBO);
    glDeleteBuffers(1, &gCurrHeightSSBO);
    glDeleteBuffers(1, &gCurrDXSSBO);
    glDeleteBuffers(1, &gCurrDYSSBO);

    glDeleteProgram(gWaveSpectrumCS);
    glDeleteProgram(gHorizontalIFFTCS);
    glDeleteProgram(gVerticalIFFTCS);

    cleanupSkybox();
    cleanupOceanFloor();
    cleanupBubbleTexture();
    cleanupOceanNormalTexture();

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
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSwapInterval(1);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    initSpectrum();      // 스펙트럼 초기화
    initComputeShader(); // compute 셰이더 & gBaseSSBO/gTempSSBO/gCurrSSBO 준비
    initPointDraw();     // point 렌더링 셰이더 + VAO 준비
    initSkybox();        // 스카이박스 초기화
    initOceanFloor();    // 파도 바닥 초기화
    initBubbleTexture();
    initOceanNormalTexture();

    lastFrameTime = glfwGetTime();

    // 메인 루프
    while (!glfwWindowShouldClose(window)) {
        double currentFrameTime = glfwGetTime();
        double deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        float currentTime = (float) currentFrameTime * timeScale;

        // GPU 파이프라인을 실행해서 gCurrHeightSSBO 채우기
        calcPipeline(currentTime);

        // Get window size for aspect ratio
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        // 화면 클리어 + 렌더 + 버퍼 스왑
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Calculate view and projection matrices (same as in drawIFFTPoints)
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        float aspect = (float)windowWidth / (float)windowHeight;
        glm::mat4 projection = glm::perspective(glm::radians(75.0f), aspect, 0.1f, 100.0f);

        // Draw skybox first (with depth test modifications inside)
        drawSkybox(view, projection);

        // Draw ocean
        drawIFFTPoints(currentTime, windowWidth, windowHeight);

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    // 정리
    cleanup();
    glfwTerminate();

    return 0;
}
