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

//////////////////////// 커스텀 함수 ////////////////////////
using BulletVec = std::vector<EnemyBullet>;
using BulletPattern = std::function<BulletVec(GameState &, int, int)>;
using PatternEntry = std::pair<BulletPattern, int>; // {패턴함수, 시작시각(ms)}

float basePosFunc(int t, float speed) { return 0; }
float sqrtPosFunc1(int t, float speed) {
    float deltaX = static_cast<float>(t) * speed;
    return std::sqrt(deltaX);
}
float sqrtPosFunc2(int t, float speed) {
    float deltaX = static_cast<float>(t) * speed;
    return -std::sqrt(3.0f * deltaX);
}

BulletVec bossEmptyPattern(GameState &gameState, int /*currentTime*/, int bossNum) {
    switch (bossNum) {
    case 1:
        gameState.bossObject1.coolTimePeriod = 300;
        break;
    case 2:
        gameState.bossObject2.coolTimePeriod = 300;
        break;
    default:
        break;
    }
    return {};
}
BulletVec bossBulletPattern1(GameState &gameState, int currentTime, int bossNum) {
    switch (bossNum) {
    case 1:
        gameState.bossObject1.coolTimePeriod = 300;
        break;
    case 2:
        gameState.bossObject2.coolTimePeriod = 300;
        break;
    default:
        break;
    }
    BulletVec bullets;
    static bool isFunc1 = false;
    int bulletCount = 13;
    bullets.reserve(bulletCount);
    constexpr float SPEED = 0.0003f;
    const glm::fvec2 CENTER = (bossNum == 1) ? gameState.bossObject1.currentPosition
                                             : gameState.bossObject2.currentPosition;

    for (int i = 0; i < bulletCount; ++i) {
        float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) /
                      static_cast<float>(bulletCount);
        glm::fvec2 dir(std::cos(angle), std::sin(angle));
        bullets.emplace_back(dir, CENTER, SPEED, currentTime,
                             isFunc1 ? sqrtPosFunc1 : sqrtPosFunc2);
    }
    isFunc1 = !isFunc1;
    return bullets;
}

static const std::vector<PatternEntry> BOSS_PATTERN_LIST = {{
    {bossEmptyPattern, 0},
    {bossBulletPattern1, 3000},
}};
static std::size_t bossPatternListCounter = 0;
///////////////////////////////////////////////////////////

BulletPattern getCurrentBulletPattern(int currentTime) {
    static BulletPattern current = bossEmptyPattern;

    const std::vector<PatternEntry> &currentBossPatternList = BOSS_PATTERN_LIST;

    while (bossPatternListCounter < currentBossPatternList.size() &&
           currentTime >= currentBossPatternList[bossPatternListCounter].second) {
        current = currentBossPatternList[bossPatternListCounter].first;
        ++bossPatternListCounter;
    }
    return current;
}

bool Boss::update(int currentTime, GameState &gameState) {
    if (isDying) {
        // Update fragments
        int deltaTime = currentTime - deathStartTime;
        std::erase_if(fragments, [deltaTime, &gameState](auto &fragment) {
            return fragment.update(deltaTime, gameState);
        });

        // Show victory screen after 3 seconds
        if ((currentTime - deathStartTime) > 3000) {
            showVictoryScreen(gameState);
            std::exit(0);
        }

        // Boss is completely destroyed after 6 seconds (won't reach here due to exit)
        return (currentTime - deathStartTime) > 5000;
    }

    // Check if boss should die
    if (gameState.bossHealth <= 0 && !isDying) {
        startDeathAnimation(currentTime);
        startCameraShake(currentTime);
        return false;
    }

    this->currentPosition = this->currentMove.getCurrentPosition(currentTime);

    // Don't damage player if boss is already dying
    if (!isDying && !gameState.playerObject.isInvincible &&
        detectCollision(*this, gameState.playerObject)) {
        gameState.playerHealth -= 1;
        gameState.playerObject.takeDamage(currentTime);
        startCameraShake(currentTime);
        if (gameState.playerHealth < 0)
            gameState.playerHealth = 0;
    }

    if (this->coolTime > currentTime)
        return false;
    this->coolTime = currentTime + this->coolTimePeriod;

    auto newBullets1 = getCurrentBulletPattern(currentTime)(gameState, currentTime, 1);
    auto newBullets2 = getCurrentBulletPattern(currentTime)(gameState, currentTime, 2);
    gameState.enemyBulletObjects.insert(gameState.enemyBulletObjects.end(), newBullets1.begin(),
                                        newBullets1.end());
    gameState.enemyBulletObjects.insert(gameState.enemyBulletObjects.end(), newBullets2.begin(),
                                        newBullets2.end());

    return false;
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
    glMatrixMode(GL_MODELVIEW);

    gameState.backgroundObject.draw(gameState.cameraOffset, gameState);

    // Draw trail particles before bullets for better visual effect
    for (auto &particle : gameState.trailParticles) {
        particle.draw(gameState.cameraOffset, gameState);
    }

    for (auto &object : gameState.enemyBulletObjects) {
        object.draw(gameState.cameraOffset, gameState);
    }
    for (auto &object : gameState.playerBulletObjects) {
        object.draw(gameState.cameraOffset, gameState);
    }
    gameState.playerObject.draw(gameState.cameraOffset, gameState);
    gameState.bossObject1.draw(gameState.cameraOffset, gameState);
    gameState.bossObject2.draw(gameState.cameraOffset, gameState);

    gameState.bossHealthBarObject.draw(gameState.cameraOffset, gameState);
    gameState.heartsObject.draw(gameState.cameraOffset, gameState);

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

//////////////////////// 커스텀 함수 ////////////////////////
using MoveFn = std::function<BossMove(int)>;
using MoveEntry = std::pair<MoveFn, int>;

auto traj1 = [](float u) { return u * (1.0f - u); };
auto por1 = [](float t) { return float(3 * t * t - 2 * t * t * t); };
MoveFn boss1Move1 = [](int currentTime) {
    return BossMove(gameState.bossObject1.currentPosition, glm::fvec2(0.0f, 0.0f), 3000,
                    currentTime, traj1, por1);
};

MoveFn boss2Move1 = [](int currentTime) {
    return BossMove(gameState.bossObject2.currentPosition, glm::fvec2(0.0f, 0.6f), 3000,
                    currentTime, traj1, por1);
};

auto zeroTraj = [](float u) { return 0.0f; };

MoveFn boss1RandomMove = [](int currentTime) {
    float randomX = dist(gen) * 1.7f - 0.85f;
    float randomY = dist(gen) * 0.85f;

    return BossMove(gameState.bossObject1.currentPosition, glm::fvec2(randomX, randomY), 1000,
                    currentTime, zeroTraj, por1);
};
MoveFn boss2RandomMove = [](int currentTime) {
    float randomX = dist(gen) * 1.7f - 0.85f;
    float randomY = dist(gen) * 0.85f;

    return BossMove(gameState.bossObject2.currentPosition, glm::fvec2(randomX, randomY), 1000,
                    currentTime, zeroTraj, por1);
};

static const std::vector<MoveEntry> BOSS_MOVE_LIST1 = {{
    {boss1Move1, 2000},
}};
static const std::vector<MoveEntry> BOSS_MOVE_LIST2 = {{
    {boss2Move1, 2000},
}};
static std::size_t boss1MoveListCounter = 0;
static std::size_t boss2MoveListCounter = 0;
///////////////////////////////////////////////////////////

int random1Iteration = 0;
int random2Iteration = 0;
std::optional<BossMove> getCurrentMove(int currentTime, int bossNum) {
    std::size_t &bossMoveListCounter = (bossNum == 1) ? boss1MoveListCounter : boss2MoveListCounter;
    int &randomIteration = (bossNum == 1) ? random1Iteration : random2Iteration;
    const std::vector<MoveEntry> &currentBossMoveList =
        (bossNum == 1) ? BOSS_MOVE_LIST1 : BOSS_MOVE_LIST2;

    if (bossMoveListCounter >= currentBossMoveList.size()) {
        if (currentTime > 11000 + randomIteration * 5000) {
            randomIteration += 1;
            return (bossNum == 1) ? boss1RandomMove(currentTime) : boss2RandomMove(currentTime);
        }
        return std::nullopt;
    }

    const auto &[makeMove, startAt] = currentBossMoveList[bossMoveListCounter];

    if (currentTime >= startAt) {
        BossMove move = makeMove(currentTime);
        ++bossMoveListCounter;
        return move;
    }

    return std::nullopt;
}

void timer(int) {
    int now = glutGet(GLUT_ELAPSED_TIME); // Get Time in milliseconds.
    static int lastMs = now;

    auto bossMoveData1 = getCurrentMove(now, 1);
    auto bossMoveData2 = getCurrentMove(now, 2);
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
