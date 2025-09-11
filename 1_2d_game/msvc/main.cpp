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

struct Bullet : Updatable, Drawable, Collidable {
    glm::fvec2 direction;

    Bullet(glm::fvec2 direction) {
        this->direction = direction;
    }
    ~Bullet() {}

    bool update(int t) {}
    void draw(glm::fvec2 cameraOffset) {}
    CollisionShape getCollisionShape() {}
};

struct PlayerBullet : Updatable, Drawable, Collidable {
    PlayerBullet() {}
    ~PlayerBullet() {}

    bool update(int t) {}
    void draw(glm::fvec2 cameraOffset) {}
    CollisionShape getCollisionShape() {}
};

struct Player : Updatable, Drawable, Collidable {
    Player() {}
    ~Player() {}

    bool update(int t) {}
    void draw(glm::fvec2 cameraOffset) {}
    CollisionShape getCollisionShape() {}
};

struct Boss : Updatable, Drawable, Collidable {
    Boss() {}
    ~Boss() {}

    bool update(int t) {}
    void draw() {}
    CollisionShape getCollisionShape() {}
};

struct Hearts : Drawable {
    Hearts() {}
    ~Hearts() {}

    void draw() {}
};

struct HealthBar : Drawable {
    HealthBar() {}
    ~HealthBar() {}

    void draw() {}
};

using Object = std::variant<Player, Boss, Bullet, PlayerBullet, Hearts, HealthBar>;

struct GameState {
    int health;
    int bossHealth;
    glm::fvec2 playerPosition;
    glm::fvec2 cameraOffset;
    std::vector<Object> sprites;
};

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO

    glutSwapBuffers();
    glutPostRedisplay();
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

    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}
