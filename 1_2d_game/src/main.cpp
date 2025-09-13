#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <functional>
#include <vector>
#include <algorithm>
#include "collision.hpp"
#include "utils.hpp"

/// @brief Interface for objects that can be drawn
struct Drawable {
    /// @brief Draw the object with a given camera camera_offset
    /// @param camera_offset The camera offset to apply
    virtual void draw(glm::vec2 camera_offset) = 0;
    virtual ~Drawable() = default;
};

bool keyStates[256] = {false};

struct GameState;
struct BossMove;

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
    void draw(glm::fvec2 cameraOffset) override {
        drawCircle(currentPosition - cameraOffset, 0.03f, 10, glm::fvec3(1.0f, 1.0f, 1.0f));
    }
    CollisionShape getShape() const override {
        return CollisionCircle(glm::vec2(0.0f, 0.0f), 1.0f);
    }
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
    void draw(glm::fvec2 cameraOffset) override {
        drawRect(currentPosition - cameraOffset, 0.03f, glm::fvec3(1.0f, 0.0f, 1.0f));
    }
    CollisionShape getShape() const override {
        return CollisionCircle(glm::vec2(0.0f, 0.0f), 1.0f);
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
    void draw(glm::fvec2 cameraOffset) override {
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
        return CollisionCircle(glm::vec2(0.0f, 0.0f), 1.0f);
    }
};

struct BossMove {
    glm::fvec2 origin;
    glm::fvec2 destination;
    glm::fvec2 directionVector;
    glm::fvec2 normalVector;
    int travelTime;
    int initialTime;
    std::function<float(float)> trajectory; // [0.0, 1.0] -> R
    std::function<float(float)> portion;    // [0.0, 1.0] -> [0.0, 1.0]

    BossMove(glm::fvec2 origin, glm::fvec2 destination, int travelTime, int initialTime,
             std::function<float(float)> trajectory, std::function<float(float)> portion)
        : origin(origin), destination(destination), travelTime(travelTime),
          initialTime(initialTime), trajectory(trajectory), portion(portion) {
        directionVector = destination - origin;
        normalVector = glm::normalize(glm::fvec2(-directionVector.y, directionVector.x));
    }

    glm::fvec2 getCurrentPosition(int currentTime) {
        if (currentTime <= initialTime)
            return origin;
        if (currentTime >= initialTime + travelTime)
            return destination;
        float timePortion = portion((currentTime - initialTime) / ((float)travelTime));
        glm::fvec2 currentPosition =
            origin + directionVector * timePortion + trajectory(timePortion) * normalVector;
        return currentPosition;
    }
};

static BossMove idleBossMove(glm::fvec2 position, int startTime = 0) {
    auto trivialFunc = [](float) { return 0; };
    return BossMove(position, position, 0, startTime, trivialFunc, trivialFunc);
}

struct Boss : Updatable, Drawable, Collidable {
    glm::fvec2 currentPosition;
    BossMove currentMove;
    int coolTime = 0;

    Boss(glm::fvec2 initialPosition)
        : currentPosition(initialPosition), currentMove(idleBossMove(initialPosition)) {}
    ~Boss() override {}

    bool update(int currentTime, GameState &gameState) override;
    void draw(glm::fvec2 cameraOffset) override {
        drawCircle(currentPosition - cameraOffset, 0.05f, 20, glm::fvec3(0.1f, 0.0f, 1.0f));
    }
    CollisionShape getShape() const override {
        return CollisionCircle(glm::vec2(0.0f, 0.0f), 1.0f);
    }
};

struct Hearts : Drawable {
    glm::fvec2 drawPosition;

    Hearts(glm::fvec2 drawPosition) : drawPosition(drawPosition) {}
    ~Hearts() override {}

    void draw(glm::fvec2 cameraOffset) override {}
};

struct BossHealthBar : Drawable {
    glm::fvec2 drawPosition;

    BossHealthBar(glm::fvec2 drawPosition) : drawPosition(drawPosition) {}
    ~BossHealthBar() override {}

    void draw(glm::fvec2 cameraOffset) override {}
};

struct GameState {
    GameState(int h, int bh)
        : health(h), bossHealth(bh), cameraOffset(0.0f, 0.0f),
          playerObject(glm::fvec2(0.0f, -0.8f)), bossObject(glm::fvec2(0.0f, 0.6f)),
          bossHealthBarObject(glm::fvec2(0.0f, 0.0f)), heartsObject(glm::fvec2(0.0f, 0.0f)) {}

    int health;
    int bossHealth;
    glm::fvec2 cameraOffset;

    Player playerObject;
    Boss bossObject;
    BossHealthBar bossHealthBarObject;
    Hearts heartsObject;

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
float sqrtPosFunc1(int t, float speed) {
    float deltaX = static_cast<float>(t) * speed;
    return std::sqrt(deltaX);
};

float sqrtPosFunc2(int t, float speed) {
    float deltaX = static_cast<float>(t) * speed;
    return -std::sqrt(2 * deltaX);
};

std::vector<EnemyBullet> bossBulletPattern1(GameState &gameState, int currentTime) {
    std::vector<EnemyBullet> bullets;
    static bool isFunc1 = false;
    bullets.reserve(30);

    const int bulletCount = 30;
    const float speed = 0.001f;
    const glm::fvec2 center = gameState.bossObject.currentPosition;

    for (int i = 0; i < bulletCount; ++i) {
        float angle = 2.0f * std::numbers::pi * i / bulletCount;
        glm::fvec2 dir(std::cos(angle), std::sin(angle));
        bullets.emplace_back(dir, center, speed, currentTime,
                             isFunc1 ? sqrtPosFunc1 : sqrtPosFunc2);
    }

    isFunc1 = !isFunc1;

    return bullets;
}
///////////////////////////////////////////////////////////

std::function<std::vector<EnemyBullet>(GameState &, int)> getCurrentBulletPattern(int currentTime) {
    return bossBulletPattern1;
}

bool Boss::update(int currentTime, GameState &gameState) {
    this->currentPosition = this->currentMove.getCurrentPosition(currentTime);

    if (this->coolTime > currentTime)
        return false;
    this->coolTime = currentTime + 500;

    auto newBullets = getCurrentBulletPattern(currentTime)(gameState, currentTime);
    gameState.enemyBulletObjects.insert(gameState.enemyBulletObjects.end(), newBullets.begin(),
                                        newBullets.end());

    std::cout << currentTime << ", " << gameState.bossHealth << ", "
              << gameState.enemyBulletObjects.size() << '\n';
    return false;
}
GameState gameState(100, 500);

void keyboardDown(unsigned char key, int /*x*/, int /*y*/) { keyStates[key] = true; }
void keyboardUp(unsigned char key, int /*x*/, int /*y*/) { keyStates[key] = false; }

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto &object : gameState.enemyBulletObjects) {
        object.draw(gameState.cameraOffset);
    }
    for (auto &object : gameState.playerBulletObjects) {
        object.draw(gameState.cameraOffset);
    }
    gameState.playerObject.draw(gameState.cameraOffset);
    gameState.bossObject.draw(gameState.cameraOffset);

    glutSwapBuffers();
    glutPostRedisplay();
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
    if (keyStates['e']) { // Camera Shake
        gameState.playerObject.tryAttack();
    }
}

void timer(int) {
    static int lastMs = 0;

    int now = glutGet(GLUT_ELAPSED_TIME); // Get Time in milliseconds.
    if (lastMs == 0) {
        glm::fvec2 origin = gameState.bossObject.currentPosition;
        glm::fvec2 dest = glm::fvec2(0.0f, 0.0f);

        auto traj = [](float u) { return u * (1.0f - u); }; // y=x(1-x) 궤적
        auto por = [](float t) {
            return 3 * t * t - 2 * t * t * t;
        }; // ease-in & ease-out 예시. por 함수는 무조건 f(1)=1, f(0)=0이어야 됨.
        // now + 2000 (2초 뒤에 시작), 3000 (3초 동안), traj을 por 순서로 따라간다. 이때, 시작
        // 지점은 origin, 도착지점은 dest이다.
        gameState.bossObject.currentMove = BossMove(origin, dest, 3000, now + 2000, traj, por);
        lastMs = now;
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
