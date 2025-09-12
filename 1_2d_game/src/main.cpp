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

/// @brief Interface for objects that can be updated
struct Updatable {
    /// @brief Update the object's state. Return true if the object should be removed.
    /// @param deltaTime Time elapsed since the last update in milliseconds
    /// @return true if the object should be removed
    virtual bool update(int deltaTime) = 0;
    virtual ~Updatable() = default;
};

struct EnemyBullet : Updatable, Drawable, Collidable {
    glm::fvec2 initialDirection;
    glm::fvec2 initialPosition;

    EnemyBullet(glm::fvec2 initialDirection, glm::fvec2 initialPosition)
        : initialDirection(initialDirection), initialPosition(initialPosition) {}
    ~EnemyBullet() {}

    bool update(int t) override { return false; }
    void draw(glm::fvec2 cameraOffset) override {}
    CollisionShape getShape() const override { return CollisionCircle(glm::vec2(0.0, 0.0), 1.0); }
};

struct PlayerBullet : Updatable, Drawable, Collidable {
    glm::fvec2 initialPosition;

    PlayerBullet(glm::fvec2 initialPosition) : initialPosition(initialPosition) {}
    ~PlayerBullet() {}

    bool update(int t) override { return false; }
    void draw(glm::fvec2 cameraOffset) override {}
    CollisionShape getShape() const override { return CollisionCircle(glm::vec2(0.0, 0.0), 1.0); }
};

struct Player : Updatable, Drawable, Collidable {
    glm::fvec2 currentPosition;

    Player(glm::fvec2 initialPosition) : currentPosition(initialPosition) {}
    ~Player() {}

    bool update(int t) override { return false; }
    void draw(glm::fvec2 cameraOffset) override { 
        // Camera Offset
        float x = currentPosition.x - cameraOffset.x;
        float y = currentPosition.y - cameraOffset.y;

        drawTriangle(glm::fvec2(x, y), 0.1f, glm::fvec3(1.0f, 1.0f, 0.0f));
    }
    void move(glm::fvec2 deltaPosition) {
        currentPosition += deltaPosition;

        float minX = -1.0f;
        float minY = -1.0f;
        float maxX = 1.0f;
        float maxY = 1.0f;

        // Clamp
        if (currentPosition.x < minX)
            currentPosition.x = minX;
        if (currentPosition.x > maxX)
            currentPosition.x = maxX;
        if (currentPosition.y < minY)
            currentPosition.y = minY;
        if (currentPosition.y > maxY)
            currentPosition.y = maxY;
    }
    CollisionShape getShape() const override { return CollisionCircle(glm::vec2(0.0, 0.0), 1.0); }
};

struct Boss : Updatable, Drawable, Collidable {
    glm::fvec2 currentPosition;

    Boss(glm::fvec2 initialPosition) : currentPosition(initialPosition) {}
    ~Boss() {}

    int cooltime = 0;

    std::vector<EnemyBullet> bullets;
    std::vector<EnemyBullet> getBullet() {
        return bullets;
    }
    bool update(int t) override {
        if (cooltime > t)
            return false;
        cooltime = t + 10000;

        

        std::cout << t << std::endl;
        return false;
    }
    void draw(glm::fvec2 cameraOffset) override {
        // Camera Offset
        float x = currentPosition.x - cameraOffset.x;
        float y = currentPosition.y - cameraOffset.y;

        drawCircle(glm::fvec2(x, y), 0.05, 20, glm::fvec3(0.1f, 0.0f, 1.0f));
    }
    CollisionShape getShape() const override { return CollisionCircle(glm::vec2(0.0, 0.0), 1.0); }
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

using Object = std::variant<Drawable *, Collidable *, Updatable *>;

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

GameState gameState(100, 500);

void keyboardDown(unsigned char key, int x, int y) { keyStates[key] = true; }
void keyboardUp(unsigned char key, int x, int y) { keyStates[key] = false; }

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gameState.playerObject.draw(gameState.cameraOffset);
    gameState.bossObject.draw(gameState.cameraOffset);
    for (auto& object : gameState.enemyBulletObjects) {
        object.draw(gameState.cameraOffset);
    }

    glutSwapBuffers();
    glutPostRedisplay();
}

float playerSpeedBase = 0.5f;

void keyInputUpdate(float dt) {
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
}

void timer(int) {
    static int last_ms = 0;
    static int start_ms = 0;

    int now = glutGet(GLUT_ELAPSED_TIME); // Get Time in milliseconds.
    if (last_ms == 0) {
        last_ms = now;
        start_ms = now;
    }
    
    float dt = (now - last_ms) / 1000.0f;
    int gameTime = now - start_ms;
    
    last_ms = now;

    gameState.bossObject.update(gameTime);
    
    keyInputUpdate(dt);
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(600, 600);
    glutCreateWindow("FreeGLUT + GLEW + GLM Example 2");

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
