#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <variant>
#include <concepts>
#include <vector>
#include <optional>

struct Drawable {
    /// Virtual function to draw the object
    virtual void draw(glm::vec2 camera_offset) = 0;
    virtual ~Drawable() = default;
};

bool keyStates[256] = {false};

// Forward declarations for collision shapes
struct CollisionCircle;
struct CollisionRectangle;
using CollisionShape = std::variant<CollisionCircle, CollisionRectangle>;

// Concrete shape implementations
struct CollisionCircle {
    glm::vec2 center;
    float raidus;
    CollisionCircle(glm::vec2 c, float r) : center(c), raidus(r) {}

    bool intersects(const CollisionCircle &other) const {
        // TODO: Implement circle-circle collision
        return false;
    }
    bool intersects(const CollisionRectangle &rect) const {
        // TODO: Implement circle-rectangle collision
        return false;
    }
};

struct CollisionRectangle {
    glm::vec2 topLeft;
    glm::vec2 bottomRight;
    CollisionRectangle(glm::vec2 tl, glm::vec2 br) : topLeft(tl), bottomRight(br) {}

    bool intersects(const CollisionCircle &circle) const {
        // TODO: Implement rectangle-circle collision
        return false;
    }
    bool intersects(const CollisionRectangle &other) const {
        // TODO: Implement rectangle-rectangle collision
        return false;
    }
};

// Interface for objects that can participate in collision detection
struct Collidable {
    virtual CollisionShape getShape() const = 0;
    virtual ~Collidable() = default;
};

// Concept for shapes that support intersection tests
template <typename T>
concept ShapeConcept = requires(const T &a, const CollisionCircle &c, const CollisionRectangle &r) {
    { a.intersects(c) } -> std::same_as<bool>;
    { a.intersects(r) } -> std::same_as<bool>;
};

// Direct shape-to-shape collision detection
template <ShapeConcept A, ShapeConcept B> bool detectCollision(const A &a, const B &b) {
    return a.intersects(b);
}

// Concept for collidable objects
template <typename T>
concept CollidableObject = std::is_base_of_v<Collidable, T>;

// Object-to-object collision detection (using their shapes)
template <CollidableObject A, CollidableObject B> bool detectCollision(const A &a, const B &b) {
    const CollisionShape shapeA = a.getShape();
    const CollisionShape shapeB = b.getShape();

    return std::visit([](const auto &s1, const auto &s2) { return s1.intersects(s2); }, shapeA,
                      shapeB);
}

struct Updatable {
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
    void draw(glm::fvec2 cameraOffset) override {}
    CollisionShape getShape() const override { return CollisionCircle(glm::vec2(0.0, 0.0), 1.0); }
};

struct Boss : Updatable, Drawable, Collidable {
    glm::fvec2 currentPosition;
    
    Boss(glm::fvec2 initialPosition) : currentPosition(initialPosition) {}
    ~Boss() {}

    bool update(int t) override { return false; }
    void draw(glm::fvec2 cameraOffset) override {}
    CollisionShape getShape() const override { return CollisionCircle(glm::vec2(0.0, 0.0), 1.0); }
};

struct Hearts : Drawable {
    glm::fvec2 drawPosition;

    Hearts(glm::fvec2 drawPosition) : drawPosition(drawPosition) {}
    ~Hearts() {}

    void draw(glm::fvec2 cameraOffset) override {}
};

struct HealthBar : Drawable {
    glm::fvec2 drawPosition;

    HealthBar(glm::fvec2 drawPosition) : drawPosition(drawPosition) {}
    ~HealthBar() {}

    void draw(glm::fvec2 cameraOffset) override {}
};

using Object = std::variant<Drawable *, Collidable *, Updatable *>;

struct GameState {
    GameState(int h, int bh)
        : health(h), bossHealth(bh), cameraOffset(0.0f, 0.0f), playerSprite(glm::fvec2(0.0f, 0.0f)),
          bossSprite(glm::fvec2(0.0f, 0.0f)), healthBarSprite(glm::fvec2(0.0f, 0.0f)), heartsSprite(glm::fvec2(0.0f, 0.0f)) {}

    int health;
    int bossHealth;
    glm::fvec2 cameraOffset;

    Player playerSprite;
    Boss bossSprite;
    HealthBar healthBarSprite;
    Hearts heartsSprite;

    std::vector<PlayerBullet> playerBulletSprites;
    std::vector<EnemyBullet> enemyBulletSprites;
};

void keyboardDown(unsigned char key, int x, int y) { keyStates[key] = true; }
void keyboardUp(unsigned char key, int x, int y) { keyStates[key] = false; }

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO

    glutSwapBuffers();
    glutPostRedisplay();
}

void update() {
    if (keyStates[27]) {
        std::cout << "ESC pressed -> exit\n";
        glutLeaveMainLoop(); // End Main Loop
    }
    if (keyStates['a']) {
        std::cout << "a clicked" << std::endl;
    }
    if (keyStates['w']) {
        std::cout << "w clicked" << std::endl;
    }
}

void timer(int value) {
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

    GameState gameState(100, 500);

    glEnable(GL_DEPTH_TEST);

    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}
