#include "utils.hpp"
#include "base.hpp"

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

GameState::GameState(int h, int bh)
    : MAX_PLAYER_HEALTH(h), MAX_BOSS_HEALTH(bh), playerHealth(h), bossHealth(bh),
      cameraOffset(0.0f, 0.0f), playerObject(glm::fvec2(0.0f, -0.8f)),
      bossObject1(glm::fvec2(0.5f, 0.6f), 1), bossObject2(glm::fvec2(-0.5f, 0.6f), 2),
      bossHealthBarObject(glm::fvec2(0.0f, 0.0f)), heartsObject(glm::fvec2(0.0f, 0.0f)) {}

float playerSpeedBase = 0.0005f; // f/ms
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

// Konami Command: up, up, down, down, left, right, left, right, B, A
// This command is widely known in gaming culture for granting special
CommandExecutor commandExecutor({'w', 'w', 's', 's', 'a', 'd', 'a', 'd', 'b', 'a'},
                                [](GameState &gameState) {
                                    // Prevent activation if player is dying
                                    if (gameState.playerObject.isDying)
                                        return;
                                    // Activate Konami command effects
                                    gameState.MAX_PLAYER_HEALTH = 10;
                                    gameState.playerHealth = 10;
                                    gameState.konamiUsed = true;

                                    // Grant 5 seconds of invincibility
                                    int currentTime = glutGet(GLUT_ELAPSED_TIME);
                                    gameState.playerObject.isInvincible = true;
                                    gameState.playerObject.invincibilityEndTime =
                                        currentTime + 5000; // 5 seconds

                                    // Upgrade to 5 bullets
                                    gameState.playerObject.bulletCount = 5;

                                    std::cout << "↑↑↓↓←→←→BA" << '\n';
                                });

void keyboardDown(unsigned char key, int /*x*/, int /*y*/) { keyStates[key] = true; }
void keyboardUp(unsigned char key, int /*x*/, int /*y*/) { keyStates[key] = false; }

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glPushMatrix();
    glTranslatef(-gameState.cameraOffset.x, -gameState.cameraOffset.y, 0.0f);
    glMatrixMode(GL_MODELVIEW);

    gameState.backgroundObject.draw(gameState);

    // Draw trail particles before bullets for better visual effect
    for (auto &particle : gameState.trailParticles) {
        particle.draw(gameState);
    }

    for (auto &object : gameState.enemyBulletObjects) {
        object.draw(gameState);
    }
    for (auto &object : gameState.playerBulletObjects) {
        object.draw(gameState);
    }
    gameState.playerObject.draw(gameState);
    gameState.bossObject1.draw(gameState);
    gameState.bossObject2.draw(gameState);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

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

    // Reset tilt angle when no left/right movement
    bool movingHorizontal = false;

    if (keyStates['a']) {
        gameState.playerObject.move(glm::vec2(-playerSpeed, 0.0f));
        gameState.playerObject.targetTiltAngle = 25.0f; // Roll left when moving left
        movingHorizontal = true;
    }
    if (keyStates['s']) {
        gameState.playerObject.move(glm::vec2(0.0f, -playerSpeed));
    }
    if (keyStates['d']) {
        gameState.playerObject.move(glm::vec2(playerSpeed, 0.0f));
        gameState.playerObject.targetTiltAngle = -25.0f; // Roll right when moving right
        movingHorizontal = true;
    }

    // Return to neutral position when not moving horizontally
    if (!movingHorizontal) {
        gameState.playerObject.targetTiltAngle = 0.0f;
    }

    if (keyStates[' ']) {
        gameState.playerObject.tryAttack();
    }
}

void timer(int) {
    int now = glutGet(GLUT_ELAPSED_TIME); // Get Time in milliseconds.
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
        gameState.cameraOffset = cameraShake(now);
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
    // Force window size to remain 800x800
    if (width != 800 || height != 800) {
        glutReshapeWindow(800, 800);
    }
    glViewport(0, 0, 800, 800);
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 800);
    glutCreateWindow("CSED451 Assn 2");

    // Test GLM properly linked
    glm::vec3 glmTest(1.0f, 0.0f, 0.0f);

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW 초기화 실패: " << glewGetErrorString(err) << '\n';
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}
