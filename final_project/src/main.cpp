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

GLuint createIslandShaderProgram() {
    GLint success = GL_FALSE;
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint prog = glCreateProgram();

    glShaderSource(vs, 1, &shaders::ISLAND_VERT_SHADER, nullptr);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[128];
        glGetShaderInfoLog(vs, 128, nullptr, log);
        std::cerr << "Island Vertex Shader Compile Failure:\n" << log << '\n';
    }

    glShaderSource(fs, 1, &shaders::ISLAND_FRAG_SHADER, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[128];
        glGetShaderInfoLog(fs, 128, nullptr, log);
        std::cerr << "Island Fragment Shader Compile Failure:\n" << log << '\n';
    }

    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char log[128];
        glGetProgramInfoLog(prog, 128, nullptr, log);
        std::cerr << "Island Shader Program Link Failure:\n" << log << '\n';
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
GLuint gIslandProgram = 0;

// camera state (shoreline view)
glm::vec3 cameraPos = glm::vec3(1.0f, 0.5f, 1.0f);
glm::vec3 cameraTarget = glm::vec3(-3.0f, 0.0f, -3.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

void initProgram() {
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
    gIslandProgram = createIslandShaderProgram();
    glBindVertexArray(0);
}

void drawIsland(int windowWidth, int windowHeight) {
    glUseProgram(gIslandProgram);
    glBindVertexArray(gPointVAO);

    // SSBO 바인딩
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gTerrainHeightSSBO);

    // Grid and scale uniforms
    GLint locGridSize = glGetUniformLocation(gIslandProgram, "uGridSize");
    GLint locRenderGrid = glGetUniformLocation(gIslandProgram, "uRenderGridSize");
    GLint locHeightScale = glGetUniformLocation(gIslandProgram, "uHeightScale");
    GLint locView = glGetUniformLocation(gIslandProgram, "uView");
    GLint locProjection = glGetUniformLocation(gIslandProgram, "uProjection");
    GLint locCameraPos = glGetUniformLocation(gIslandProgram, "uCameraPos");

    glUniform1i(locGridSize, GRID_SIZE);
    glUniform1i(locRenderGrid, RENDER_GRID_SIZE);
    glUniform1f(locHeightScale, TERRAIN_HEIGHT_SCALE);

    // View matrix (looking at ocean from angle)
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    glUniformMatrix4fv(locView, 1, GL_FALSE, &view[0][0]);

    // Projection matrix
    float aspect = (float)windowWidth / (float)windowHeight;
    glm::mat4 projection = glm::perspective(glm::radians(75.0f), aspect, 0.1f, 100.0f);
    glUniformMatrix4fv(locProjection, 1, GL_FALSE, &projection[0][0]);

    // PBR uniforms
    glUniform3fv(locCameraPos, 1, &cameraPos[0]);

    // Bind island normal map
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gIslandNormalTexture);
    GLint locNormalMap = glGetUniformLocation(gIslandProgram, "uNormalMap");
    glUniform1i(locNormalMap, 4);

    // Draw filled triangles
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    int numIndices = (RENDER_GRID_SIZE - 1) * (RENDER_GRID_SIZE - 1) * 6;

    // Calculate camera forward vector for back-face culling
    glm::vec3 cameraForward = glm::normalize(cameraTarget - cameraPos);
    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0); // Draw this tile

    glBindVertexArray(0);
    glUseProgram(0);
}

void drawIFFTPoints(float currentTime, int windowWidth, int windowHeight) {
    glUseProgram(gPointProgram);
    glBindVertexArray(gPointVAO);

    // SSBO 바인딩
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gFinalZSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gCurrTessenHeightSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, gCurrDXSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, gCurrDYSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, gBlendMaskSSBO);

    // Grid and scale uniforms
    GLint locIFFTGrid = glGetUniformLocation(gPointProgram, "uIFFTGridSize");
    GLint locRenderGrid = glGetUniformLocation(gPointProgram, "uRenderGridSize");
    GLint locScale = glGetUniformLocation(gPointProgram, "uHeightScale");
    GLint locModel = glGetUniformLocation(gPointProgram, "uModel");
    GLint locCenterTile = glGetUniformLocation(gPointProgram, "uCenterTile");
    glUniform1i(locIFFTGrid, GRID_SIZE);
    glUniform1i(locRenderGrid, RENDER_GRID_SIZE);
    glUniform1f(locScale, HEIGHT_SCALE);

    // Lambda (수평 변위 강도)
    GLint locLambda = glGetUniformLocation(gPointProgram, "uLambda");
    glUniform1f(locLambda, lambda);

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

    // Draw tiled ocean
    const int tileRadius = 1;
    const float tileSize = 4.0f;

    // Calculate camera forward vector for back-face culling
    glm::vec3 cameraForward = glm::normalize(cameraTarget - cameraPos);

    for (int tz = -tileRadius; tz <= tileRadius; tz++) {
        for (int tx = -tileRadius; tx <= tileRadius; tx++) {
            // Tile center in world space
            glm::vec3 tileCenter = glm::vec3(tx * tileSize, 0.0f, tz * tileSize);

            if (tx == 0 && tz == 0) {
                glUniform1i(locCenterTile, 1);
            } else {
                glUniform1i(locCenterTile, 0);
            }

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
}

// free resources
void cleanup() {
    glDeleteVertexArrays(1, &gPointVAO);
    glDeleteBuffers(1, &gPointEBO);
    glDeleteProgram(gPointProgram);

    glDeleteBuffers(1, &gInitSpectrumSSBO);
    glDeleteBuffers(1, &gInitSpectrumConjSSBO);
    glDeleteBuffers(1, &gCurrSpectrumSSBO);
    glDeleteBuffers(1, &gIFFTTempSSBO);
    glDeleteBuffers(1, &gCurrTessenHeightSSBO);
    glDeleteBuffers(1, &gCurrDXSSBO);
    glDeleteBuffers(1, &gCurrDYSSBO);

    glDeleteBuffers(1, &gTerrainHeightSSBO);
    glDeleteBuffers(1, &gSpongeMaskSSBO);
    glDeleteBuffers(1, &gBlendMaskSSBO);
    glDeleteBuffers(1, &gFinalZSSBO);
    glDeleteBuffers(1, &gHeightASSBO);
    glDeleteBuffers(1, &gVelUASSBO);
    glDeleteBuffers(1, &gHeightBSSBO);
    glDeleteBuffers(1, &gVelUBSSBO);
    glDeleteBuffers(1, &gVelVBSSBO);

    glDeleteProgram(gWaveSpectrumCS);
    glDeleteProgram(gHorizontalIFFTCS);
    glDeleteProgram(gVerticalIFFTCS);
    glDeleteProgram(gPDESolverCS);

    cleanupSkybox();
    cleanupOceanFloor();
    cleanupBubbleTexture();
    cleanupOceanNormalTexture();
    cleanupIslandNormalTexture();

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
    initProgram();       // point 렌더링 셰이더 + VAO 준비
    initSkybox();        // 스카이박스 초기화
    initOceanFloor();    // 파도 바닥 초기화
    initBubbleTexture();
    initOceanNormalTexture();
    initIslandNormalTexture();

    // 물리 연산 한 단계의 시간 (0.005초)
    const float FIXED_DT = 0.005f;
    const int MAX_SUB_STEPS = 10; // 렌더링이 너무 느려져도 한 프레임에 물리 연산을 10번 넘게 하지 않음
    static double accumulator = 0.0;
    static float simulationTime = 0.0f;

    // 초기화가 끝난 직후의 시간이 기준점
    glfwSetTime(0.0);
    double startTime = glfwGetTime();
    lastFrameTime = startTime;

    // 메인 루프
    while (!glfwWindowShouldClose(window)) {
        double realTime = glfwGetTime();
        double deltaTime = realTime - lastFrameTime;
        lastFrameTime = realTime;

        // 창 이동 중과 같이 프레임 드랍이 심할 때 DT가 튀는 것 방지
        if (deltaTime > 0.1)
            deltaTime = 0.1;

        accumulator += deltaTime * timeScale;

        // 누적된 시간이 고정 시간(FIXED_DT)보다 크다면, 그만큼 시뮬레이션을 더 진행
        int steps = 0;
        while (accumulator >= FIXED_DT && steps < MAX_SUB_STEPS) {
            simulationTime += FIXED_DT;
            calcPipeline(simulationTime);
            accumulator -= FIXED_DT;
            steps++;
        }

        // 렌더링
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        // 화면 클리어 + 렌더 + 버퍼 스왑
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        float aspect = (float)windowWidth / (float)windowHeight;
        glm::mat4 projection = glm::perspective(glm::radians(75.0f), aspect, 0.1f, 100.0f);

        drawSkybox(view, projection);

        // 렌더링 셰이더에도 simulationTime을 넘겨주어 물결 위상이 맞게 한다
        drawIFFTPoints(simulationTime, windowWidth, windowHeight);

        drawIsland(windowWidth, windowHeight);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 정리
    cleanup();
    glfwTerminate();

    return 0;
}
