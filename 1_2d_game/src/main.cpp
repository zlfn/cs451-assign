#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <functional>
#include <optional>
#include <utility>
#include <vector>
#include <utility>
#include <array>
#include "collision.hpp"
#include "utils.hpp"

struct GameState;
bool keyStates[256] = {false};
struct BossMove;

/// @brief Interface for objects that can be drawn
struct Drawable {
    /// @brief Draw the object with a given camera camera_offset
    /// @param camera_offset The camera offset to apply
    virtual void draw(glm::vec2 camera_offset, const GameState &gameState) = 0;
    virtual ~Drawable() = default;
};

/// @brief Interface for objects that can be updated
struct Updatable {
    /// @brief Update the object's state. Return true if the object should be removed.
    /// @param deltaTime Time elapsed since the last update in milliseconds
    /// @return true if the object should be removed
    virtual bool update(int currentTime, GameState &gameState) = 0;
    virtual ~Updatable() = default;
};

struct EnemyBullet : Updatable, Drawable, Collidable {
    glm::fvec2 initialDirection;
    glm::fvec2 normalDirection;
    glm::fvec2 initialPosition;
    glm::fvec2 currentPosition;
    int initialTime;
    float speed;
    std::function<float(int, float)> posFunc;

    EnemyBullet(glm::fvec2 initialDirection, glm::fvec2 initialPosition, float speed,
                int initialTime, std::function<float(int, float)> posFunc)
        : initialDirection(glm::normalize(initialDirection) * speed),
          normalDirection(glm::normalize(glm::fvec2(-initialDirection.y, initialDirection.x))),
          initialPosition(initialPosition), currentPosition(initialPosition),
          initialTime(initialTime), speed(speed), posFunc(std::move(posFunc)) {}
    ~EnemyBullet() override {}

    bool update(int currentTime, GameState &gameState) override {
        int dt = currentTime - initialTime;
        currentPosition =
            initialPosition + float(dt) * initialDirection + posFunc(dt, speed) * normalDirection;
        return abs(currentPosition.x) > 1.0f || abs(currentPosition.y) > 1.0f;
    }
    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override {
        drawCircle(currentPosition - cameraOffset, 0.03f, 10, glm::fvec3(1.0f, 1.0f, 1.0f));
    }
    CollisionShape getShape() const override { return CollisionCircle(currentPosition, 0.03f); }
};

struct PlayerBullet : Updatable, Drawable, Collidable {
    glm::fvec2 initialPosition;
    glm::fvec2 currentPosition;
    int initialTime;
    float speed;

    PlayerBullet(glm::fvec2 initialPosition, float speed, int initialTime)
        : initialPosition(initialPosition), currentPosition(initialPosition),
          initialTime(initialTime), speed(speed) {}
    ~PlayerBullet() override {}

    bool update(int currentTime, GameState &gameState) override {
        currentPosition =
            initialPosition + glm::fvec2(0, speed * static_cast<float>(currentTime - initialTime));
        return abs(currentPosition.x) > 1.0f || abs(currentPosition.y) > 1.0f;
        ;
    }
    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override {
        drawRect(currentPosition - cameraOffset, 0.03f, glm::fvec3(1.0f, 0.0f, 1.0f));
    }
    CollisionShape getShape() const override {
        return CollisionRectangle(currentPosition - glm::fvec2(0.015f, 0.015f),
                                  currentPosition + glm::fvec2(0.015f, 0.015f));
    }
};

struct Player : Updatable, Drawable, Collidable {
    glm::fvec2 currentPosition;
    bool isBullet = false;
    int coolTime = 0;

    Player(glm::fvec2 initialPosition) : currentPosition(initialPosition) {}
    ~Player() override {}

    void tryAttack() { isBullet = true; }
    bool update(int currentTime, GameState &gameState) override;
    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override {
        drawTriangle(currentPosition - cameraOffset, 0.1f, glm::fvec3(1.0f, 1.0f, 0.0f));
    }
    void move(glm::fvec2 deltaPosition) {
        currentPosition += deltaPosition;

        // Clamp
        if (currentPosition.x < -1.0f)
            currentPosition.x = -1.0f;
        if (currentPosition.x > 1.0f)
            currentPosition.x = 1.0f;
        if (currentPosition.y < -1.0f)
            currentPosition.y = -1.0f;
        if (currentPosition.y > 1.0f)
            currentPosition.y = 1.0f;
    }
    CollisionShape getShape() const override {
        return CollisionRectangle(currentPosition - glm::fvec2(0.05f, 0.05f),
                                  currentPosition + glm::fvec2(0.05f, 0.05f));
    }
};

constexpr bool approxEqual(float a, float b, float epsilon = 1e-2f) {
    return (a > b ? a - b : b - a) < epsilon;
}

/// @brief Concept for functions f(0)=f(1)=0
/// @tparam F Function type
template <typename F>
concept Map00Fn = requires {
    { F{}(0.0f) } -> std::same_as<float>;
    { F{}(1.0f) } -> std::same_as<float>;
} && approxEqual(F{}(0.0f), 0.0f) && approxEqual(F{}(1.0f), 0.0f);

/// @brief Concept for functions f(0)=0, f(1)=1
/// @tparam F Function type
template <typename F>
concept Map01Fn = requires {
    { F{}(0.0f) } -> std::same_as<float>;
    { F{}(1.0f) } -> std::same_as<float>;
} && approxEqual(F{}(0.0f), 0.0f) && approxEqual(F{}(1.0f), 1.0f);

struct BossMove {
    glm::fvec2 origin;
    glm::fvec2 destination;
    glm::fvec2 directionVector{};
    glm::fvec2 normalVector{};
    int travelTime;
    int initialTime;
    std::function<float(float)> trajectory; // f(0) = 0, f(1) = 0
    std::function<float(float)> portion;    // f(1) = 0, f(1) = 1

    template <Map00Fn TrajFn, Map01Fn PorFn>
    BossMove(glm::fvec2 origin, glm::fvec2 destination, int travelTime, int initialTime,
             TrajFn trajectory, PorFn portion)
        : origin(origin), destination(destination), travelTime(travelTime),
          initialTime(initialTime), trajectory(std::move(trajectory)), portion(std::move(portion)) {
        directionVector = destination - origin;
        normalVector = glm::normalize(glm::fvec2(-directionVector.y, directionVector.x));
    }

    glm::fvec2 getCurrentPosition(int currentTime) {
        if (currentTime <= initialTime)
            return origin;
        if (currentTime >= initialTime + travelTime)
            return destination;
        float timePortion =
            portion(static_cast<float>(currentTime - initialTime) / ((float)travelTime));
        glm::fvec2 currentPosition =
            origin + directionVector * timePortion + trajectory(timePortion) * normalVector;
        return currentPosition;
    }
};

static BossMove idleBossMove(glm::fvec2 position, int startTime = 0) {
    auto trivialFunc = [](float) { return 0.0f; };
    auto trivialFuncPor = [](float t) { return t; };
    return BossMove(position, position, 0, startTime, trivialFunc, trivialFuncPor);
}

struct Boss : Updatable, Drawable, Collidable {
    glm::fvec2 currentPosition;
    BossMove currentMove;
    int coolTime = 0;
    int coolTimePeriod = 500;

    Boss(glm::fvec2 initialPosition)
        : currentPosition(initialPosition), currentMove(idleBossMove(initialPosition)) {}
    ~Boss() override {}

    bool update(int currentTime, GameState &gameState) override;
    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override {
        drawCircle(currentPosition - cameraOffset, 0.08f, 20, glm::fvec3(0.1f, 0.0f, 1.0f));
    }
    CollisionShape getShape() const override { return CollisionCircle(currentPosition, 0.08f); }
};

struct PlayerHealthBar : Drawable {
    glm::fvec2 drawPosition;

    PlayerHealthBar(glm::fvec2 drawPosition) : drawPosition(drawPosition) {}
    ~PlayerHealthBar() override {}

    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override;
};

struct BossHealthBar : Drawable {
    glm::fvec2 drawPosition;

    BossHealthBar(glm::fvec2 drawPosition) : drawPosition(drawPosition) {}
    ~BossHealthBar() override {}

    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override;
};

struct GameState {
    GameState(int h, int bh)
        : MAX_PLAYER_HEALTH(h), MAX_BOSS_HEALTH(bh), playerHealth(h), bossHealth(bh),
          cameraOffset(0.0f, 0.0f), playerObject(glm::fvec2(0.0f, -0.8f)),
          bossObject(glm::fvec2(0.0f, 0.6f)), bossHealthBarObject(glm::fvec2(0.0f, 0.0f)),
          heartsObject(glm::fvec2(0.0f, 0.0f)) {}

    const int MAX_PLAYER_HEALTH;
    const int MAX_BOSS_HEALTH;
    int playerHealth;
    int bossHealth;
    glm::fvec2 cameraOffset;

    Player playerObject;
    Boss bossObject;
    BossHealthBar bossHealthBarObject;
    PlayerHealthBar heartsObject;

    std::vector<PlayerBullet> playerBulletObjects;
    std::vector<EnemyBullet> enemyBulletObjects;
};

bool Player::update(int currentTime, GameState &gameState) {
    if (currentTime >= this->coolTime && this->isBullet) {
        gameState.playerBulletObjects.emplace_back(this->currentPosition, 0.001f, currentTime);
        this->isBullet = false;
        this->coolTime = currentTime + 200;
    }
    return false;
}

//////////////////////// 커스텀 함수 ////////////////////////
using BulletVec = std::vector<EnemyBullet>;
using BulletPattern = std::function<BulletVec(GameState &, int)>;
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

BulletVec bossEmptyPattern(GameState &gameState, int /*currentTime*/) {
    gameState.bossObject.coolTimePeriod = 500;
    return {};
}
BulletVec bossBulletPattern1(GameState &gameState, int currentTime) {
    gameState.bossObject.coolTimePeriod = 300;
    BulletVec bullets;
    static bool isFunc1 = false;
    bullets.reserve(30);

    constexpr int BULLET_COUNT = 30;
    constexpr float SPEED = 0.0005f;
    const glm::fvec2 CENTER = gameState.bossObject.currentPosition;

    for (int i = 0; i < BULLET_COUNT; ++i) {
        float angle = 2.0f * std::numbers::pi_v<float> * float(i) / BULLET_COUNT;
        glm::fvec2 dir(std::cos(angle), std::sin(angle));
        bullets.emplace_back(dir, CENTER, SPEED, currentTime,
                             isFunc1 ? sqrtPosFunc1 : sqrtPosFunc2);
    }
    isFunc1 = !isFunc1;
    return bullets;
}
BulletVec bossBulletPattern2(GameState &gameState, int currentTime) {
    gameState.bossObject.coolTimePeriod = 400;
    BulletVec bullets;
    constexpr int BULLET_COUNT = 15;
    constexpr float SPEED = 0.0005f;
    constexpr float SPREAD_DEG = 75.0f;
    constexpr float SPREAD_RAD = glm::radians(SPREAD_DEG);

    bullets.reserve(BULLET_COUNT);

    const glm::fvec2 CENTER = gameState.bossObject.currentPosition;
    glm::fvec2 toPlayer = gameState.playerObject.currentPosition - CENTER;

    const float BASE_ANGLE = std::atan2(toPlayer.y, toPlayer.x);

    for (int i = 0; i < BULLET_COUNT; ++i) {
        float t = (BULLET_COUNT == 1) ? 0.0f : (static_cast<float>(i) / (BULLET_COUNT - 1) - 0.5f);
        float angle = BASE_ANGLE + t * SPREAD_RAD;

        glm::fvec2 dir(std::cos(angle), std::sin(angle));
        bullets.emplace_back(dir, CENTER, SPEED, currentTime, basePosFunc);
    }

    return bullets;
}
BulletVec bossBulletPattern3(GameState &gameState, int currentTime) {
    gameState.bossObject.coolTimePeriod = 200;
    BulletVec bullets;
    static int startTime = currentTime;
    constexpr int BULLET_COUNT = 4;
    constexpr float SPEED = 0.001f;
    bullets.reserve(BULLET_COUNT);

    float baseAngle = static_cast<float>(startTime - currentTime) / 1000.0f;
    const glm::fvec2 CENTER = gameState.bossObject.currentPosition;

    for (int i = 0; i < BULLET_COUNT; ++i) {
        float t = static_cast<float>(i) / (BULLET_COUNT - 1) - 0.5f;
        float angle = baseAngle + t;

        glm::fvec2 dir(std::cos(angle), std::sin(angle));
        bullets.emplace_back(dir, CENTER, SPEED, currentTime, basePosFunc);
    }

    return bullets;
}

static const std::array<PatternEntry, 6> BOSS_PATTERN_LIST = {{
    {bossBulletPattern2, 0},
    {bossBulletPattern1, 1000},
    {bossBulletPattern3, 3000},
    {bossBulletPattern1, 6000},
    {bossBulletPattern2, 8000},
    {bossEmptyPattern, 12000},
}};
static std::size_t bossPatternListCounter = 0;
///////////////////////////////////////////////////////////

BulletPattern getCurrentBulletPattern(int currentTime) {
    static int gameStartTime = currentTime;
    static BulletPattern current = bossEmptyPattern;
    const int ELAPSED_TIME = currentTime - gameStartTime;

    while (bossPatternListCounter < BOSS_PATTERN_LIST.size() &&
           ELAPSED_TIME >= BOSS_PATTERN_LIST[bossPatternListCounter].second) {
        current = BOSS_PATTERN_LIST[bossPatternListCounter].first;
        ++bossPatternListCounter;
    }
    return current;
}

bool Boss::update(int currentTime, GameState &gameState) {
    this->currentPosition = this->currentMove.getCurrentPosition(currentTime);

    if (this->coolTime > currentTime)
        return false;
    this->coolTime = currentTime + this->coolTimePeriod;

    auto newBullets = getCurrentBulletPattern(currentTime)(gameState, currentTime);
    gameState.enemyBulletObjects.insert(gameState.enemyBulletObjects.end(), newBullets.begin(),
                                        newBullets.end());

    std::cout << currentTime << ", " << gameState.bossHealth << ", "
              << gameState.enemyBulletObjects.size() << '\n';
    return false;
}

void PlayerHealthBar::draw(glm::fvec2 cameraOffset, const GameState &gameState) {
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int maxHealth = gameState.MAX_PLAYER_HEALTH;
    int currentHealth = gameState.playerHealth;

    // Calculate segment size based on max health to fit within half screen width
    float maxTotalWidth = 1.0f; // Half of screen width
    float spacing = 0.015f;
    float totalSpacing = spacing * static_cast<float>(maxHealth - 1);
    float availableWidth = maxTotalWidth - totalSpacing - 0.05f; // Leave small margin
    float segmentWidth = availableWidth / static_cast<float>(maxHealth);

    // Limit segment width to prevent too large segments
    segmentWidth = glm::min(segmentWidth, 0.2f);

    // Recalculate total width with actual segment width
    // totalWidth is calculated but not currently used

    // Position at bottom-left corner of screen
    float startX = -0.975f; // Near left edge
    float startY = -0.95f;  // Near bottom edge
    float segmentHeight = 0.04f;
    float zDepth = 0.9f;

    // Draw rectangle segments for each health point
    for (int i = 0; i < maxHealth; i++) {
        float x = startX + static_cast<float>(i) * (segmentWidth + spacing) + segmentWidth / 2.0f;

        glm::fvec4 color;
        if (i < currentHealth) {
            // Active health - bright orange with gradient
            float intensity =
                0.8f + 0.2f * std::sinf(static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.003f +
                                        static_cast<float>(i) * 0.5f);
            color = glm::fvec4(1.0f, 0.5f * intensity, 0.1f, 0.9f);
        } else {
            // Lost health - dark gray
            color = glm::fvec4(0.2f, 0.2f, 0.2f, 0.5f);
        }

        // Draw the rectangle with glow
        drawRectWithGlow(x, startY, segmentWidth, segmentHeight, color,
                         (i < currentHealth) ? 0.02f : 0.0f, zDepth);
    }

    glDisable(GL_BLEND);
    glPopMatrix();
}

void BossHealthBar::draw(glm::fvec2 cameraOffset, const GameState &gameState) {
    float healthPercentage =
        static_cast<float>(gameState.bossHealth) / static_cast<float>(gameState.MAX_BOSS_HEALTH);
    healthPercentage = glm::clamp(healthPercentage, 0.0f, 1.0f);

    float barWidth = 1.9f; // Almost full screen width
    float barHeight = 0.03f;
    float barX = 0.0f;
    float barY = 0.95f;
    float zDepth = 0.9f;

    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw background bar
    glBegin(GL_QUADS);
    glColor4f(0.2f, 0.2f, 0.2f, 0.7f);
    glVertex3f(-barWidth / 2, barY - barHeight / 2, zDepth);
    glVertex3f(barWidth / 2, barY - barHeight / 2, zDepth);
    glVertex3f(barWidth / 2, barY + barHeight / 2, zDepth);
    glVertex3f(-barWidth / 2, barY + barHeight / 2, zDepth);
    glEnd();

    // Draw health bar with glow using the utility function
    float healthBarWidth = barWidth * healthPercentage;
    float healthBarX = -barWidth / 2 + healthBarWidth / 2;

    drawRectWithGlow(healthBarX, barY, healthBarWidth, barHeight,
                     glm::fvec4(0.5f, 0.8f, 1.0f, 1.0f), 0.04f, 1.0f);

    glDisable(GL_BLEND);

    glPopMatrix();
}

GameState gameState(5, 500);

void keyboardDown(unsigned char key, int /*x*/, int /*y*/) { keyStates[key] = true; }
void keyboardUp(unsigned char key, int /*x*/, int /*y*/) { keyStates[key] = false; }

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    for (auto &object : gameState.enemyBulletObjects) {
        object.draw(gameState.cameraOffset, gameState);
    }
    for (auto &object : gameState.playerBulletObjects) {
        object.draw(gameState.cameraOffset, gameState);
    }
    gameState.playerObject.draw(gameState.cameraOffset, gameState);
    gameState.bossObject.draw(gameState.cameraOffset, gameState);

    gameState.bossHealthBarObject.draw(gameState.cameraOffset, gameState);
    gameState.heartsObject.draw(gameState.cameraOffset, gameState);

    glutSwapBuffers();
    glutPostRedisplay();
}

bool isCameraShake = false;
glm::fvec2 cameraShake(int currentTime) {
    static int cameraShakeStartTime = currentTime;
    int deltaTime = currentTime - cameraShakeStartTime;
    if (deltaTime > 2000) {
        isCameraShake = false;
    }
    float offset =
        0.5f / (static_cast<float>(deltaTime) / 2.0f - 20.0f * std::numbers::pi_v<float>)*std::sin(
                   static_cast<float>(deltaTime) / 2.0f - 20.0f * std::numbers::pi_v<float>);
    return glm::fvec2(offset, 0.0);
}

float playerSpeedBase = 0.0005f; // f/ms

void keyInputUpdate(int dt) {
    float playerSpeed = playerSpeedBase * static_cast<float>(dt);
    if (keyStates[27]) {
        std::cout << "ESC pressed -> exit\n";
        std::exit(0);
    }
    if (keyStates['w']) {
        gameState.playerObject.move(glm::vec2(0.0f, playerSpeed));
    }
    if (keyStates['a']) {
        gameState.playerObject.move(glm::vec2(-playerSpeed, 0.0f));
    }
    if (keyStates['s']) {
        gameState.playerObject.move(glm::vec2(0.0f, -playerSpeed));
    }
    if (keyStates['d']) {
        gameState.playerObject.move(glm::vec2(playerSpeed, 0.0f));
    }
    if (keyStates[' ']) {
        gameState.playerObject.tryAttack();
    }
    if (keyStates['e']) { // Camera Shake Sample
        isCameraShake = true;
    }
}

//////////////////////// 커스텀 함수 ////////////////////////
using MoveFn = std::function<BossMove(int)>;
using MoveEntry = std::pair<MoveFn, int>;

auto traj1 = [](float u) { return u * (1.0f - u); }; // y=x(1-x) 궤적. 무조건 f(0)=f(1)=0이어야 함.
auto por1 = [](float t) {
    return float(3 * t * t - 2 * t * t * t);
}; // ease-in & ease-out 예시. por 함수는 무조건 f(0)=0, f(1)=1이어야 됨.
// now + 2000 (2초 뒤에 시작), 3000 (3초 동안), traj을 por 순서로 따라간다. 이때, 시작
// 지점은 origin, 도착지점은 dest이다.
MoveFn bossMove1 = [](int currentTime) {
    return BossMove(gameState.bossObject.currentPosition, glm::fvec2(0.0f, 0.0f), 3000, currentTime,
                    traj1, por1);
};

MoveFn bossMove2 = [](int currentTime) {
    return BossMove(gameState.bossObject.currentPosition, glm::fvec2(0.0f, 0.6f), 3000, currentTime,
                    traj1, por1);
};

static std::array<MoveEntry, 2> bossMoveList = {{
    {bossMove1, 2000},
    {bossMove2, 7000},
}};
static std::size_t bossMoveListCounter = 0;
///////////////////////////////////////////////////////////

std::optional<BossMove> getCurrentMove(int currentTime) {
    static int gameStartTime = currentTime;
    const int ELAPSED_TIME = currentTime - gameStartTime;

    if (bossMoveListCounter >= bossMoveList.size()) {
        return std::nullopt;
    }

    const auto &[makeMove, startAt] = bossMoveList[bossMoveListCounter];

    if (ELAPSED_TIME >= startAt) {
        BossMove move = makeMove(currentTime);
        ++bossMoveListCounter;
        return move;
    }

    return std::nullopt;
}

void timer(int) {
    int now = glutGet(GLUT_ELAPSED_TIME); // Get Time in milliseconds.
    static int lastMs = now;

    auto bossMoveData = getCurrentMove(now);
    if (bossMoveData.has_value()) {
        gameState.bossObject.currentMove = bossMoveData.value();
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

    gameState.playerObject.update(now, gameState);
    gameState.bossObject.update(now, gameState);

    glutTimerFunc(16, timer, 0);
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(600, 600);
    glutCreateWindow("CSED451 Assn 1");

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
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}
