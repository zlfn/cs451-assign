#include "utils.hpp"
#include "base.hpp"
#include "graphics.hpp"

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

GameState::GameState(int h, int bh)
    : MAX_PLAYER_HEALTH(h), MAX_BOSS_HEALTH(bh), playerHealth(h), bossHealth(bh),
      cameraBaseOffset(0.0f, 0.0f), cameraShakeOffset(0.0f, 0.0f),
      playerObject(glm::fvec2(0.0f, -0.8f)), bossObject1(glm::fvec2(0.5f, 0.6f), 1),
      bossObject2(glm::fvec2(-0.5f, 0.6f), 2), bossHealthBarObject(glm::fvec2(0.0f, 0.0f)),
      heartsObject(glm::fvec2(0.0f, 0.0f)) {}

MatrixStack modelViewStack;
MatrixStack projectionStack;

enum ProjMethod { DIAG_PERSPECTIVE, TOP_PERSPECTIVE, TOP_PARALLEL };
ProjMethod currentProjMethod = DIAG_PERSPECTIVE;

enum ShadingStyle { GOURAUD, PHONG, PHONG_WITH_NORMAL };
ShadingStyle currentShadingStyle = GOURAUD;

int keyPressDelay = 0;          // For projection method changes ('c' key)
int renderModeKeyDelay = 0;     // For render mode changes ('e' key)

float playerSpeedBase = 0.00065f;
bool isCameraShake = false;
int cameraShakeStartTime = 0;
bool keyStates[256] = {false};
void showVictoryScreen(const GameState &gameState);

void startCameraShake(int currentTime) {
    isCameraShake = true;
    cameraShakeStartTime = currentTime;
}

glm::fvec2 cameraShake(int currentTime) {
    int deltaTime = currentTime - cameraShakeStartTime;
    if (deltaTime > 2000) {
        isCameraShake = false;
    }
    float offset =
        0.5f / (static_cast<float>(deltaTime) / 2.0f - 20.0f * std::numbers::pi_v<float>)*std::sin(
                   static_cast<float>(deltaTime) / 2.0f - 20.0f * std::numbers::pi_v<float>);
    return glm::fvec2(offset, 0.0);
}

GameState gameState(5, 200);

// 코나미 커맨드: ↑↑↓↓←→←→BA
CommandExecutor commandExecutor({'w', 'w', 's', 's', 'a', 'd', 'a', 'd', 'b', 'a'},
                                [](GameState &gameState) {
                                    if (gameState.playerObject.isDying)
                                        return;

                                    gameState.MAX_PLAYER_HEALTH = 10;
                                    gameState.playerHealth = 10;
                                    gameState.konamiUsed = true;

                                    int currentTime = glutGet(GLUT_ELAPSED_TIME);
                                    gameState.playerObject.isInvincible = true;
                                    gameState.playerObject.invincibilityEndTime =
                                        currentTime + 5000;
                                    gameState.playerObject.bulletCount = 5;

                                    std::cout << "↑↑↓↓←→←→BA" << '\n';
                                });

void keyboardDown(unsigned char key, int /*x*/, int /*y*/) { keyStates[key] = true; }
void keyboardUp(unsigned char key, int /*x*/, int /*y*/) { keyStates[key] = false; }

float smoothProjRotChange() {
    int dt = glutGet(GLUT_ELAPSED_TIME) - keyPressDelay;
    if (dt > 500 || currentProjMethod == TOP_PARALLEL) {
        return 0.0f;
    } else {
        return 50.0 * (1.0 - ((float)dt / 500.0));
    }
}

float smoothProjScaleChange() {
    int dt = glutGet(GLUT_ELAPSED_TIME) - keyPressDelay;
    if (dt > 500 || currentProjMethod == TOP_PARALLEL) {
        return 2.0f;
    } else {
        return 2.0 + (1.0 - ((float)dt / 500.0));
    }
}

float smoothProjZDistChange() {
    int dt = glutGet(GLUT_ELAPSED_TIME) - keyPressDelay;
    if (dt > 500 || currentProjMethod == TOP_PARALLEL) {
        return -1.0f;
    } else {
        return -0.5f - 0.5f * ((float)dt / 500.0);
    }
}

void display() {
    // 설정 값
    const float SCALE = currentProjMethod == DIAG_PERSPECTIVE
        ? 3.0 : smoothProjScaleChange();
    const float CAMERA_ANGLE_X_DEG = currentProjMethod == DIAG_PERSPECTIVE
        ? 50.0f : smoothProjRotChange(); // 카메라 회전 각도
    const float Z_DIST_VIEW = currentProjMethod == DIAG_PERSPECTIVE
        ? -0.5f : smoothProjZDistChange(); // 뷰 공간(View Space)에서의 목표 Z 거리

    // 참고: 플레이어와 뷰의 거리가 SCALE/2가 되어야 TOP_PERSPECTIVE -> TOP_PARALLEL 변환 시에 위화감이 없다.
    const float ANGLE_RAD = (float)(CAMERA_ANGLE_X_DEG * (std::numbers::pi / 180.0f));
    
    // Y 보정값
    const float Y_COMPENSATION = (std::tan(ANGLE_RAD) * std::abs(Z_DIST_VIEW)) / SCALE;

    // glTranslatef에 쓸 Z 거리. 결과가 Z_DIST_VIEW가 되도록 역산
    const float Z_TRANSLATE = Z_DIST_VIEW / std::cos(ANGLE_RAD) / SCALE;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 3D 투영
    projectionStack.loadIdentity();
    glm::mat4 projection;
    if (currentProjMethod == TOP_PARALLEL) {
        projection = glm::ortho(-SCALE, SCALE, -SCALE, SCALE, -5.0f, 5.0f);
    } else {
        projection = glm::frustum(-0.2f, 0.2f, -0.2f, 0.2f, 0.1f, 20.0f);
    }
    projectionStack.matMul(projection);

    modelViewStack.loadIdentity();

    // Helper lambda to draw skybox
    auto drawSkybox = [&]() {
        modelViewStack.matPush();
        modelViewStack.rotate(-CAMERA_ANGLE_X_DEG, 1.0f, 0.0f, 0.0f);
        gameState.skyboxObject.draw(gameState);
        modelViewStack.matPop();
    };

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    drawSkybox();

    // Now apply full camera transform for other objects
    modelViewStack.matPush();

    glm::fvec2 &cbo = gameState.cameraBaseOffset;
    modelViewStack.rotate(-CAMERA_ANGLE_X_DEG, 1.0f, 0.0f, 0.0f); // perspective view
    modelViewStack.scale(SCALE, SCALE, SCALE);
    modelViewStack.translate(-cbo.x, -cbo.y + Y_COMPENSATION, Z_TRANSLATE);
    modelViewStack.translate(-gameState.cameraShakeOffset.x, -gameState.cameraShakeOffset.y, 0.0f);

    // Helper lambda to draw polygon-based 3D objects (excluding Background which uses GL_LINES)
    auto drawPolygonObjects = [&]() {
        for (auto &particle : gameState.trailParticles) {
            particle.draw(gameState);
        }
        for (auto &object : gameState.enemyBulletObjects) {
            object.draw(gameState);
        }
        for (auto &object : gameState.playerBulletObjects) {
            object.draw(gameState);
        }
        gameState.bossObject1.draw(gameState);
        gameState.bossObject2.draw(gameState);
        gameState.playerObject.draw(gameState);
    };

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    drawPolygonObjects();
    gameState.backgroundObject.draw(gameState);

    modelViewStack.matPop(); // 3D 뷰 매트릭스 제거

    // 2D 투영
    projectionStack.loadIdentity();
    projectionStack.matMul(glm::ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0));

    // Always render 2D UI in fill mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    modelViewStack.loadIdentity();
    gameState.bossHealthBarObject.draw(gameState);
    gameState.heartsObject.draw(gameState);

    glutSwapBuffers();
    glutPostRedisplay();
}

void keyInputUpdate(int dt) {
    float playerSpeed = playerSpeedBase * static_cast<float>(dt);
    if (keyStates[27]) {
        std::cout << "ESC pressed -> exit\n";
        std::exit(0);
    }
    if (keyStates['w']) {
        gameState.playerObject.move(glm::vec2(0.0f, playerSpeed));
    }

    bool movingHorizontal = false;

    if (keyStates['a']) {
        gameState.playerObject.move(glm::vec2(-playerSpeed, 0.0f));
        gameState.playerObject.targetTiltAngle = 25.0f;
        movingHorizontal = true;
    }
    if (keyStates['s']) {
        gameState.playerObject.move(glm::vec2(0.0f, -playerSpeed));
    }
    if (keyStates['d']) {
        gameState.playerObject.move(glm::vec2(playerSpeed, 0.0f));
        gameState.playerObject.targetTiltAngle = -25.0f;
        movingHorizontal = true;
    }

    int now = glutGet(GLUT_ELAPSED_TIME);
    if (keyStates['c'] && keyPressDelay + 500 < now) {
        keyPressDelay = now;
        std::cout << "Change projection method to ";
        switch (currentProjMethod) {
        case DIAG_PERSPECTIVE:
            currentProjMethod = TOP_PERSPECTIVE;
            std::cout << "\'top perspective\'" << std::endl;
            break;
        case TOP_PERSPECTIVE:
            currentProjMethod = TOP_PARALLEL;
            std::cout << "\'top parallel\'" << std::endl;
            break;
        case TOP_PARALLEL:
            currentProjMethod = DIAG_PERSPECTIVE;
            std::cout << "\'diagonal perspective\'" << std::endl;
            break;
        }
    }

    if (keyStates['e'] && renderModeKeyDelay + 500 < now) {
        renderModeKeyDelay = now;
        std::cout << "Change shading style to ";
        switch (currentShadingStyle) {
        case GOURAUD:
            currentShadingStyle = PHONG;
            std::cout << "\'phong without normal map\'" << std::endl;
            break;
        case PHONG:
            currentShadingStyle = PHONG_WITH_NORMAL;
            std::cout << "\'phong with normal map\'" << std::endl;
            break;
        case PHONG_WITH_NORMAL:
            currentShadingStyle = GOURAUD;
            std::cout << "\'gouraud\'" << std::endl;
            break;
        }
    }

    if (!movingHorizontal) {
        gameState.playerObject.targetTiltAngle = 0.0f;
    }

    if (keyStates[' ']) {
        gameState.playerObject.tryAttack();
    }
}

void timer(int) {
    int now = glutGet(GLUT_ELAPSED_TIME);
    static int lastMs = now;

    auto bossMoveData1 = getCurrentMove(now, gameState, 1);
    auto bossMoveData2 = getCurrentMove(now, gameState, 2);
    if (bossMoveData1.has_value()) {
        gameState.bossObject1.currentMove = bossMoveData1.value();
    }
    if (bossMoveData2.has_value()) {
        gameState.bossObject2.currentMove = bossMoveData2.value();
    }
    if (isCameraShake) {
        gameState.cameraShakeOffset = cameraShake(now);
    }

    int dt = now - lastMs;
    lastMs = now;

    keyInputUpdate(dt);

    std::erase_if(gameState.enemyBulletObjects,
                  [&](auto &it) { return it.update(now, gameState); });

    std::erase_if(gameState.playerBulletObjects,
                  [&](auto &it) { return it.update(now, gameState); });

    std::erase_if(gameState.trailParticles, [&](auto &it) { return it.update(now, gameState); });

    gameState.backgroundObject.update(now, gameState);
    gameState.playerObject.update(now, gameState);
    gameState.bossObject1.update(now, gameState);
    gameState.bossObject2.update(now, gameState);

    commandExecutor.update(now, gameState);

    glutTimerFunc(16, timer, 0);
}

void reshape(int width, int height) {
    if (width != 800 || height != 800) {
        glutReshapeWindow(800, 800);
    }
    glViewport(0, 0, 800, 800);
}

// 전역 셰이더 프로그램
ShaderProgram* g_shaderProgram = nullptr;

// 셰이더 초기화 함수
void initShaders() {
    try {
        // Vertex shader 생성
        Shader vertShader = Shader::fromSource(
            Shader::Type::VERTEX,
            shaders::GOURAUD_VERT_SHADER
        );

        // Fragment shader 생성
        Shader fragShader = Shader::fromSource(
            Shader::Type::FRAGMENT,
            shaders::BASE_FRAG_SHADER
        );

        // 셰이더 프로그램 생성 및 링크
        g_shaderProgram = new ShaderProgram();
        g_shaderProgram->attachShader(vertShader);
        g_shaderProgram->attachShader(fragShader);
        g_shaderProgram->link();

        std::cout << "Shaders initialized successfully\n";
    } catch (const std::exception& e) {
        std::cerr << "Shader initialization failed: " << e.what() << '\n';
        std::exit(1);
    }
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(1400, 1400);
    glutCreateWindow("CSED451 Assn 4");

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW 초기화 실패: " << glewGetErrorString(err) << '\n';
        return -1;
    }

    // 셰이더 초기화
    initShaders();

    glEnable(GL_DEPTH_TEST);

    keyPressDelay = glutGet(GLUT_ELAPSED_TIME);
    renderModeKeyDelay = glutGet(GLUT_ELAPSED_TIME);

    // Load skybox
    if (!gameState.skyboxObject.load("assets/skybox")) {
        std::cerr << "Failed to load skybox textures\n";
    }

    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}
