#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <variant>
#include <vector>
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

    EnemyBullet(glm::fvec2 initialDirection, glm::fvec2 initialPosition, float speed,
                int initialTime)
        : initialDirection(glm::normalize(initialDirection) * speed),
          initialPosition(initialPosition), currentPosition(initialPosition), initialTime(initialTime), speed(speed) {
        normalDirection = glm::normalize(glm::fvec2(-initialDirection.y, initialDirection.x));
    }
    ~EnemyBullet() {}

    float pos(int t) {
        float deltaX = t * speed; // f/ms
        return sqrt(deltaX);
    }
    bool update(int currentTime, GameState &gameState) override {
        int dt = currentTime - initialTime;
        currentPosition =
            initialPosition + float(dt) * initialDirection + pos(dt) * normalDirection;
        return abs(currentPosition.x) > 1.0f || abs(currentPosition.y) > 1.0f;
    }
    void draw(glm::fvec2 cameraOffset) override {
        drawCircle(currentPosition - cameraOffset, 0.03f, 10, glm::fvec3(1.0f, 1.0f, 1.0f));
    }
    CollisionShape getShape() const override { return CollisionCircle(glm::vec2(0.0f, 0.0f), 1.0f); }
};

struct PlayerBullet : Updatable, Drawable, Collidable {
    glm::fvec2 initialPosition;
    glm::fvec2 currentPosition;
    int initialTime;
    float speed;

    PlayerBullet(glm::fvec2 initialPosition, float speed, int initialTime)
        : initialPosition(initialPosition), currentPosition(initialPosition),
          initialTime(initialTime),
          speed(speed) {}
    ~PlayerBullet() {}

    bool update(int currentTime, GameState &gameState) override {
        currentPosition = initialPosition + glm::fvec2(0, speed * (currentTime - initialTime));
        return abs(currentPosition.x) > 1.0f || abs(currentPosition.y) > 1.0f;;
    }
    void draw(glm::fvec2 cameraOffset) override {
        drawRect(currentPosition - cameraOffset, 0.03f, glm::fvec3(1.0f, 0.0f, 1.0f));
    }
    CollisionShape getShape() const override { return CollisionCircle(glm::vec2(0.0f, 0.0f), 1.0f); }
};

struct Player : Updatable, Drawable, Collidable {
    glm::fvec2 currentPosition;
    bool isBullet = false;
    int coolTime = 0;

    Player(glm::fvec2 initialPosition) : currentPosition(initialPosition) {}
    ~Player() {}

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
    CollisionShape getShape() const override { return CollisionCircle(glm::vec2(0.0f, 0.0f), 1.0f); }
};

struct Boss : Updatable, Drawable, Collidable {
    glm::fvec2 currentPosition;
    int cooltime = 0;

    Boss(glm::fvec2 initialPosition) : currentPosition(initialPosition) {}
    ~Boss() {}

    bool update(int currentTime, GameState &gameState) override;
    void draw(glm::fvec2 cameraOffset) override {
        drawCircle(currentPosition - cameraOffset, 0.05f, 20, glm::fvec3(0.1f, 0.0f, 1.0f));
    }
    CollisionShape getShape() const override { return CollisionCircle(glm::vec2(0.0f, 0.0f), 1.0f); }
};

struct Hearts : Drawable {
    glm::fvec2 drawPosition;

    Hearts(glm::fvec2 drawPosition) : drawPosition(drawPosition) {}
    ~Hearts() {}

    void draw(glm::fvec2 cameraOffset) override {}
};

struct BossHealthBar : Drawable {
    glm::fvec2 drawPosition;

    BossHealthBar(glm::fvec2 drawPosition) : drawPosition(drawPosition) {}
    ~BossHealthBar() {}

    void draw(glm::fvec2 cameraOffset) override {}
};

struct GameState {
    GameState(int h, int bh)
        : health(h), bossHealth(bh), cameraOffset(0.0f, 0.0f), playerObject(glm::fvec2(0.0f, 0.0f)),
          bossObject(glm::fvec2(0.0f, 0.0f)), bossHealthBarObject(glm::fvec2(0.0f, 0.0f)),
          heartsObject(glm::fvec2(0.0f, 0.0f)) {}

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

bool Player::update(int currentTime, GameState& gameState) {
    if (currentTime >= this->coolTime && this->isBullet) {
        gameState.playerBulletObjects.emplace_back(this->currentPosition, 0.001f, currentTime);
        this->isBullet = false;
        this->coolTime = currentTime + 200;
    }
    return false;
}

bool Boss::update(int currentTime, GameState &gameState) {
    if (this->cooltime > currentTime)
        return false;
    this->cooltime = currentTime + 200;

    EnemyBullet testBullet1(glm::fvec2(1.0f, 0.0f), glm::fvec2(0.0f, 0.0f), 0.001f, currentTime);
    EnemyBullet testBullet2(glm::fvec2(1.0f, 0.0f), glm::fvec2(0.0f, 0.0f), 0.002f, currentTime);
    gameState.enemyBulletObjects.push_back(testBullet1);
    gameState.enemyBulletObjects.push_back(testBullet2);

    std::cout << currentTime << ", " << gameState.bossHealth << ", "
              << gameState.enemyBulletObjects.size() << std::endl;
    return false;
}
GameState gameState(100, 500);

void keyboardDown(unsigned char key, int x, int y) { keyStates[key] = true; }
void keyboardUp(unsigned char key, int x, int y) { keyStates[key] = false; }

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
    float playerSpeed = playerSpeedBase * dt;
    if (keyStates[27]) {
        std::cout << "ESC pressed -> exit\n";
        std::exit(0);
    }
    if (keyStates['w']) {
        gameState.playerObject.move(glm::vec2(0.0f, playerSpeed));
        std::cout << "w clicked\n";
    }
    if (keyStates['a']) {
        gameState.playerObject.move(glm::vec2(-playerSpeed, 0.0f));
        std::cout << "a clicked\n";
    }
    if (keyStates['s']) {
        gameState.playerObject.move(glm::vec2(0.0f, -playerSpeed));
        std::cout << "s clicked\n";
    }
    if (keyStates['d']) {
        gameState.playerObject.move(glm::vec2(playerSpeed, 0.0f));
        std::cout << "d clicked\n";
    }
    if (keyStates['e']) { // Camera Shake
        gameState.playerObject.tryAttack();
        std::cout << "e clicked\n";
    }
}

void timer(int) {
    static int last_ms = 0;

    int now = glutGet(GLUT_ELAPSED_TIME); // Get Time in milliseconds.
    if (last_ms == 0) {
        last_ms = now;
    }
    
    int dt = now - last_ms;
    last_ms = now;
    
    keyInputUpdate(dt);

    for (auto it = gameState.enemyBulletObjects.begin();
         it != gameState.enemyBulletObjects.end();) {
        if (it->update(now, gameState)) {
            it = gameState.enemyBulletObjects.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = gameState.playerBulletObjects.begin();
         it != gameState.playerBulletObjects.end();) {
        if (it->update(now, gameState)) {
            it = gameState.playerBulletObjects.erase(it);
        } else {
            ++it;
        }
    }
    gameState.playerObject.update(now, gameState);
    gameState.bossObject.update(now, gameState);
    
    glutPostRedisplay();
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
        std::cerr << "GLEW 초기화 실패: " << glewGetErrorString(err) << std::endl;
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
