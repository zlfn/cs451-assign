#include "utils.hpp"
#include "base.hpp"

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

GameState::GameState(int h, int bh)
    : MAX_PLAYER_HEALTH(h), MAX_BOSS_HEALTH(bh), playerHealth(h), bossHealth(bh),
      cameraBaseOffset(0.0f, 0.0f), cameraShakeOffset(0.0f, 0.0f),
      playerObject(glm::fvec2(0.0f, -0.8f)), bossObject1(glm::fvec2(0.5f, 0.6f), 1),
      bossObject2(glm::fvec2(-0.5f, 0.6f), 2), bossHealthBarObject(glm::fvec2(0.0f, 0.0f)),
      heartsObject(glm::fvec2(0.0f, 0.0f)) {}

enum ProjMethod { DIAG_PERSPECTIVE, TOP_PERSPECTIVE, TOP_PARALLEL };
ProjMethod currentProjMethod = DIAG_PERSPECTIVE;
int keyPressDelay = 0;

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

GameState gameState(5, 1000);

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
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (currentProjMethod == TOP_PARALLEL) {
        glOrtho(-SCALE, SCALE, -SCALE, SCALE, -5.0f, 5.0f);
    } else {
        glFrustum(-1.0f, 1.0f, -1.0f, 1.0f, 0.5f, 20.0f);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw skybox first without camera translation (only rotation)
    glPushMatrix();
    glRotatef(-CAMERA_ANGLE_X_DEG, 1.0f, 0.0f, 0.0f);
    gameState.skyboxObject.draw(gameState);
    glPopMatrix();

    // Now apply full camera transform for other objects
    glPushMatrix();

    glm::fvec2 &cbo = gameState.cameraBaseOffset;
    glRotatef(-CAMERA_ANGLE_X_DEG, 1.0f, 0.0f, 0.0f); // perspective view
    glScalef(SCALE, SCALE, SCALE);
    glTranslatef(-cbo.x, -cbo.y + Y_COMPENSATION, Z_TRANSLATE);
    glTranslatef(-gameState.cameraShakeOffset.x, -gameState.cameraShakeOffset.y, 0.0f);
    gameState.backgroundObject.draw(gameState);
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

    glPopMatrix(); // 3D 뷰 매트릭스 제거

    // 2D 투영
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

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

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 800);
    glutCreateWindow("CSED451 Assn 3");

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW 초기화 실패: " << glewGetErrorString(err) << '\n';
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    keyPressDelay = glutGet(GLUT_ELAPSED_TIME);

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
