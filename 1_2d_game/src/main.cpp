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
#include <array>
#include <random>
#include "collision.hpp"
#include "utils.hpp"

struct GameState;
bool keyStates[256] = {false};
struct BossMove;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

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

bool isCameraShake = false;
int cameraShakeStartTime = 0;

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

    bool update(int currentTime, GameState &gameState) override;
    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override {
        glm::fvec2 pos = currentPosition - cameraOffset;
        float size = 0.03f;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        // Outer glow
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.8f, 0.2f, 1.0f, 0.3f);
        glVertex3f(pos.x, pos.y, -0.05f);
        glColor4f(0.4f, 0.0f, 0.8f, 0.0f);
        for (int i = 0; i <= 12; i++) {
            float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / 12.0f;
            float x = pos.x + size * 2.0f * std::cos(angle);
            float y = pos.y + size * 2.0f * std::sin(angle);
            glVertex3f(x, y, -0.05f);
        }
        glEnd();

        // Main diamond shape
        glBegin(GL_QUADS);
        glColor4f(1.0f, 0.3f, 0.8f, 1.0f);
        glVertex3f(pos.x, pos.y + size, 0.0f);
        glVertex3f(pos.x + size * 0.7f, pos.y, 0.0f);
        glVertex3f(pos.x, pos.y - size, 0.0f);
        glVertex3f(pos.x - size * 0.7f, pos.y, 0.0f);
        glEnd();

        // Inner core
        glBegin(GL_QUADS);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glVertex3f(pos.x, pos.y + size * 0.5f, 0.01f);
        glVertex3f(pos.x + size * 0.35f, pos.y, 0.01f);
        glVertex3f(pos.x, pos.y - size * 0.5f, 0.01f);
        glVertex3f(pos.x - size * 0.35f, pos.y, 0.01f);
        glEnd();

        // Rotating effect
        float rotation = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.005f;
        glBegin(GL_LINES);
        glLineWidth(1.5f);
        glColor4f(0.6f, 0.1f, 1.0f, 0.7f);
        for (int i = 0; i < 4; i++) {
            float angle = rotation + static_cast<float>(i) * std::numbers::pi_v<float> / 2.0f;
            glVertex3f(pos.x, pos.y, 0.02f);
            glVertex3f(pos.x + size * 1.2f * std::cos(angle), pos.y + size * 1.2f * std::sin(angle),
                       0.02f);
        }
        glEnd();

        glDisable(GL_BLEND);
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

    bool update(int currentTime, GameState &gameState) override;
    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override {
        glm::fvec2 pos = currentPosition - cameraOffset;
        float width = 0.015f;
        float height = 0.04f;
        float zDepth = 0.0f;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.9f, 0.0f, 1.0f);
        glVertex3f(pos.x, pos.y + height, zDepth);
        glColor4f(1.0f, 0.5f, 0.0f, 0.8f);
        glVertex3f(pos.x - width, pos.y - height * 0.3f, zDepth);
        glVertex3f(pos.x + width, pos.y - height * 0.3f, zDepth);
        glEnd();

        glBegin(GL_QUADS);
        glColor4f(1.0f, 0.7f, 0.0f, 1.0f);
        glVertex3f(pos.x - width * 0.6f, pos.y - height * 0.2f, zDepth);
        glVertex3f(pos.x + width * 0.6f, pos.y - height * 0.2f, zDepth);
        glColor4f(1.0f, 0.3f, 0.0f, 0.2f);
        glVertex3f(pos.x + width * 0.4f, pos.y - height * 1.5f, zDepth);
        glVertex3f(pos.x - width * 0.4f, pos.y - height * 1.5f, zDepth);
        glEnd();

        glBegin(GL_POINTS);
        glPointSize(8.0f);
        glColor4f(1.0f, 1.0f, 0.7f, 0.9f);
        glVertex3f(pos.x, pos.y + height * 0.7f, zDepth);
        glEnd();

        glDisable(GL_BLEND);
    }
    CollisionShape getShape() const override {
        return CollisionRectangle(currentPosition - glm::fvec2(0.015f, 0.015f),
                                  currentPosition + glm::fvec2(0.015f, 0.015f));
    }
};

struct PlayerFragment : Drawable, Updatable {
    glm::fvec2 position;
    glm::fvec2 velocity;
    float rotation;
    float rotationSpeed;
    float size;
    float alpha;
    glm::fvec3 color;

    PlayerFragment(glm::fvec2 pos, glm::fvec2 vel, float rot, float rotSpeed, float sz,
                   glm::fvec3 col)
        : position(pos), velocity(vel), rotation(rot), rotationSpeed(rotSpeed), size(sz),
          alpha(1.0f), color(col) {}

    bool update(int deltaTime, GameState &) override {
        float dt = static_cast<float>(deltaTime) * 0.0005f;
        position += velocity * dt;
        rotation += rotationSpeed * dt;
        velocity.y -= 0.4f * dt;
        alpha -= dt * 0.2f;
        size *= (1.0f - dt * 0.15f);
        return alpha <= 0.0f || size <= 0.001f;
    }

    void draw(glm::fvec2 cameraOffset, const GameState &) override {
        glm::fvec2 pos = position - cameraOffset;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glPushMatrix();
        glTranslatef(pos.x, pos.y, 0.0f);
        glRotatef(rotation, 0.0f, 0.0f, 1.0f);

        glBegin(GL_TRIANGLES);
        glColor4f(color.r, color.g, color.b, alpha);
        glVertex3f(0.0f, size, 0.0f);
        glColor4f(color.r * 0.5f, color.g * 0.5f, color.b * 0.5f, alpha * 0.5f);
        glVertex3f(-size * 0.866f, -size * 0.5f, 0.0f);
        glVertex3f(size * 0.866f, -size * 0.5f, 0.0f);
        glEnd();

        glPopMatrix();
        glDisable(GL_BLEND);
    }
};

struct Player : Updatable, Drawable, Collidable {
    glm::fvec2 currentPosition;
    bool isBullet = false;
    int coolTime = 0;
    bool isInvincible = false;
    int invincibilityEndTime = 0;
    float tiltAngle = 0.0f;                             // Rotation angle for tilting
    float targetTiltAngle = 0.0f;                       // Target angle for smooth transition
    static constexpr int INVINCIBILITY_DURATION = 1200; // 1.2 seconds of invincibility
    bool isDying = false;
    int deathStartTime = 0;
    std::vector<PlayerFragment> fragments;
    int bulletCount = 3; // Number of bullets to fire at once

    Player(glm::fvec2 initialPosition) : currentPosition(initialPosition) {}
    ~Player() override {}

    void startDeathAnimation(int currentTime) {
        if (isDying)
            return;
        isDying = true;
        deathStartTime = currentTime;

        // Create fragments
        std::uniform_real_distribution<float> speedDist(0.4f, 1.5f);
        std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * std::numbers::pi_v<float>);
        std::uniform_real_distribution<float> rotSpeedDist(-360.0f, 360.0f);
        std::uniform_real_distribution<float> sizeDist(0.02f, 0.06f);

        for (int i = 0; i < 15; ++i) {
            float speed = speedDist(gen);
            float angle = angleDist(gen);
            glm::fvec2 vel(speed * std::cos(angle), speed * std::sin(angle));
            float rotSpeed = rotSpeedDist(gen);
            float size = sizeDist(gen);

            glm::fvec3 color;
            if (i % 3 == 0) {
                color = glm::fvec3(1.0f, 1.0f, 0.0f); // Yellow
            } else if (i % 3 == 1) {
                color = glm::fvec3(1.0f, 0.6f, 0.0f); // Orange
            } else {
                color = glm::fvec3(1.0f, 0.8f, 0.2f); // Light orange
            }

            fragments.emplace_back(currentPosition, vel, 0.0f, rotSpeed, size, color);
        }
    }

    void tryAttack() {
        if (!isDying)
            isBullet = true;
    }
    bool update(int currentTime, GameState &gameState) override;
    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override {
        if (isDying) {
            // Draw fragments
            for (auto &fragment : fragments) {
                fragment.draw(cameraOffset, gameState);
            }

            // Add explosion effect
            int currentTime = glutGet(GLUT_ELAPSED_TIME);
            float timeSinceDeath = static_cast<float>(currentTime - deathStartTime) * 0.001f;
            if (timeSinceDeath < 1.2f) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);

                float explosionSize = 0.2f * (1.0f + timeSinceDeath * 3.0f);
                float alpha = 1.0f - timeSinceDeath * 0.83f;

                glm::fvec2 pos = currentPosition - cameraOffset;
                glBegin(GL_TRIANGLE_FAN);
                glColor4f(1.0f, 0.9f, 0.0f, alpha * 0.8f);
                glVertex3f(pos.x, pos.y, 0.0f);
                glColor4f(1.0f, 0.5f, 0.0f, 0.0f);
                for (int i = 0; i <= 20; i++) {
                    float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / 20.0f;
                    float x = pos.x + explosionSize * std::cos(angle);
                    float y = pos.y + explosionSize * std::sin(angle);
                    glVertex3f(x, y, 0.0f);
                }
                glEnd();

                glDisable(GL_BLEND);
            }
            return;
        }

        glPushMatrix();
        // Apply rotation for rolling effect (Y-axis rotation)
        glTranslatef(currentPosition.x - cameraOffset.x, currentPosition.y - cameraOffset.y, 0.0f);
        glRotatef(tiltAngle, 0.0f, 1.0f, 0.0f);
        glTranslatef(-(currentPosition.x - cameraOffset.x), -(currentPosition.y - cameraOffset.y),
                     0.0f);

        // Semi-transparent rendering during invincibility
        if (isInvincible) {
            // Flashing effect during invincibility
            float alpha =
                0.3f +
                0.4f * std::abs(std::sin(static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.01f));
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            drawSpaceship(currentPosition - cameraOffset, 0.14f,
                          glm::fvec4(1.0f, 1.0f, 0.0f, alpha));
            glDisable(GL_BLEND);
        } else {
            drawSpaceship(currentPosition - cameraOffset, 0.14f,
                          glm::fvec4(1.0f, 1.0f, 0.0f, 1.0f));
        }
        glPopMatrix();
    }
    void move(glm::fvec2 deltaPosition) {
        if (isDying)
            return; // No movement when dying

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
    void takeDamage(int currentTime) {
        if (!isInvincible) {
            isInvincible = true;
            invincibilityEndTime = currentTime + INVINCIBILITY_DURATION;
        }
    }
    CollisionShape getShape() const override {
        if (isDying) {
            // No collision when dying
            return CollisionCircle(glm::fvec2(-999.0f, -999.0f), 0.0f);
        }
        return CollisionRectangle(currentPosition - glm::fvec2(0.015f, 0.015f + 0.025f),
                                  currentPosition + glm::fvec2(0.015f, 0.015f - 0.025f));
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

struct BossFragment : Drawable, Updatable {
    glm::fvec2 position;
    glm::fvec2 velocity;
    float rotation;
    float rotationSpeed;
    float size;
    float alpha;
    glm::fvec3 color;

    BossFragment(glm::fvec2 pos, glm::fvec2 vel, float rot, float rotSpeed, float sz,
                 glm::fvec3 col)
        : position(pos), velocity(vel), rotation(rot), rotationSpeed(rotSpeed), size(sz),
          alpha(1.0f), color(col) {}

    bool update(int deltaTime, GameState &) override {
        float dt = static_cast<float>(deltaTime) * 0.0005f; // Slower animation
        position += velocity * dt;
        rotation += rotationSpeed * dt;
        velocity.y -= 0.3f * dt;    // Slower gravity
        alpha -= dt * 0.15f;        // Slower fade out
        size *= (1.0f - dt * 0.1f); // Slower shrink
        return alpha <= 0.0f || size <= 0.001f;
    }

    void draw(glm::fvec2 cameraOffset, const GameState &) override {
        glm::fvec2 pos = position - cameraOffset;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glPushMatrix();
        glTranslatef(pos.x, pos.y, 0.0f);
        glRotatef(rotation, 0.0f, 0.0f, 1.0f);

        glBegin(GL_TRIANGLES);
        glColor4f(color.r, color.g, color.b, alpha);
        glVertex3f(0.0f, size, 0.0f);
        glColor4f(color.r * 0.5f, color.g * 0.5f, color.b * 0.5f, alpha * 0.5f);
        glVertex3f(-size * 0.866f, -size * 0.5f, 0.0f);
        glVertex3f(size * 0.866f, -size * 0.5f, 0.0f);
        glEnd();

        glPopMatrix();
        glDisable(GL_BLEND);
    }
};

struct Boss : Updatable, Drawable, Collidable {
    glm::fvec2 currentPosition;
    BossMove currentMove;
    int coolTime = 0;
    int coolTimePeriod = 500;
    bool isDying = false;
    int deathStartTime = 0;
    std::vector<BossFragment> fragments;

    Boss(glm::fvec2 initialPosition)
        : currentPosition(initialPosition), currentMove(idleBossMove(initialPosition)) {}
    ~Boss() override {}

    void startDeathAnimation(int currentTime) {
        if (isDying)
            return;
        isDying = true;
        deathStartTime = currentTime;

        // Create fragments
        std::uniform_real_distribution<float> speedDist(0.3f, 1.2f); // Slower speed
        std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * std::numbers::pi_v<float>);
        std::uniform_real_distribution<float> rotSpeedDist(-180.0f, 180.0f); // Slower rotation
        std::uniform_real_distribution<float> sizeDist(0.03f, 0.1f);         // Bigger fragments

        for (int i = 0; i < 20; ++i) {
            float speed = speedDist(gen);
            float angle = angleDist(gen);
            glm::fvec2 vel(speed * std::cos(angle), speed * std::sin(angle));
            float rotSpeed = rotSpeedDist(gen);
            float size = sizeDist(gen);

            glm::fvec3 color;
            if (i % 3 == 0) {
                color = glm::fvec3(1.0f, 0.2f, 0.8f); // Pink
            } else if (i % 3 == 1) {
                color = glm::fvec3(0.6f, 0.1f, 1.0f); // Purple
            } else {
                color = glm::fvec3(0.8f, 0.4f, 1.0f); // Light purple
            }

            fragments.emplace_back(currentPosition, vel, 0.0f, rotSpeed, size, color);
        }
    }

    bool update(int currentTime, GameState &gameState) override;
    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override {
        if (isDying) {
            // Draw fragments
            for (auto &fragment : fragments) {
                fragment.draw(cameraOffset, gameState);
            }

            // Add explosion effect
            int currentTime = glutGet(GLUT_ELAPSED_TIME);
            float timeSinceDeath = static_cast<float>(currentTime - deathStartTime) * 0.001f;
            if (timeSinceDeath < 1.5f) { // Longer explosion effect
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);

                float explosionSize = 0.3f * (1.0f + timeSinceDeath * 2.0f); // Slower expansion
                float alpha = 1.0f - timeSinceDeath * 0.67f;                 // Slower fade

                glm::fvec2 pos = currentPosition - cameraOffset;
                glBegin(GL_TRIANGLE_FAN);
                glColor4f(1.0f, 0.8f, 1.0f, alpha * 0.8f);
                glVertex3f(pos.x, pos.y, 0.0f);
                glColor4f(0.8f, 0.2f, 1.0f, 0.0f);
                for (int i = 0; i <= 20; i++) {
                    float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / 20.0f;
                    float x = pos.x + explosionSize * std::cos(angle);
                    float y = pos.y + explosionSize * std::sin(angle);
                    glVertex3f(x, y, 0.0f);
                }
                glEnd();

                glDisable(GL_BLEND);
            }
            return;
        }
        glm::fvec2 pos = currentPosition - cameraOffset;
        float size = 0.15f;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Main body - octagon shape
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.3f, 0.0f, 0.8f, 1.0f);
        glVertex3f(pos.x, pos.y, 0.0f);
        glColor4f(0.1f, 0.0f, 0.6f, 1.0f);
        for (int i = 0; i <= 8; i++) {
            float angle = static_cast<float>(i) * std::numbers::pi_v<float> / 4.0f;
            float x = pos.x + size * std::cos(angle);
            float y = pos.y + size * std::sin(angle);
            glVertex3f(x, y, 0.0f);
        }
        glEnd();

        // Core glow
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.8f, 0.2f, 1.0f, 0.8f);
        glVertex3f(pos.x, pos.y, 0.01f);
        glColor4f(0.4f, 0.0f, 0.8f, 0.2f);
        for (int i = 0; i <= 20; i++) {
            float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / 20.0f;
            float x = pos.x + size * 0.6f * std::cos(angle);
            float y = pos.y + size * 0.6f * std::sin(angle);
            glVertex3f(x, y, 0.01f);
        }
        glEnd();

        // Rotating spikes
        float rotation = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.001f;
        glBegin(GL_TRIANGLES);
        for (int i = 0; i < 6; i++) {
            float angle = rotation + static_cast<float>(i) * std::numbers::pi_v<float> / 3.0f;

            glColor4f(0.6f, 0.1f, 1.0f, 0.9f);
            glVertex3f(pos.x, pos.y, 0.02f);

            glColor4f(0.2f, 0.0f, 0.4f, 0.6f);
            glVertex3f(pos.x + size * 1.3f * std::cos(angle), pos.y + size * 1.3f * std::sin(angle),
                       0.02f);
            glVertex3f(pos.x + size * 0.8f * std::cos(angle + 0.2f),
                       pos.y + size * 0.8f * std::sin(angle + 0.2f), 0.02f);
        }
        glEnd();

        // Eye or core detail
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.0f, 0.0f, 0.5f, 1.0f);
        glVertex3f(pos.x, pos.y, 0.03f);
        glColor4f(0.3f, 0.0f, 0.2f, 1.0f);
        for (int i = 0; i <= 10; i++) {
            float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / 10.0f;
            float x = pos.x + size * 0.3f * std::cos(angle);
            float y = pos.y + size * 0.3f * std::sin(angle);
            glVertex3f(x, y, 0.03f);
        }
        glEnd();

        // Energy field effect
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        glColor4f(0.4f, 0.2f, 1.0f, 0.5f);
        for (int i = 0; i < 16; i++) {
            float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / 16.0f;
            float wobble = std::sin(rotation * 3.0f + angle * 2.0f) * 0.02f;
            float x = pos.x + (size * 1.1f + wobble) * std::cos(angle);
            float y = pos.y + (size * 1.1f + wobble) * std::sin(angle);
            glVertex3f(x, y, 0.0f);
        }
        glEnd();

        glDisable(GL_BLEND);
    }
    CollisionShape getShape() const override {
        if (isDying) {
            // No collision when dying
            return CollisionCircle(glm::fvec2(-999.0f, -999.0f), 0.0f);
        }
        return CollisionCircle(currentPosition, 0.15f);
    }
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

struct Star : Drawable, Updatable {
    glm::fvec2 position;
    float speed;
    float size;
    float brightness;

    Star(glm::fvec2 pos, float spd, float sz, float br)
        : position(pos), speed(spd), size(sz), brightness(br) {}

    bool update(int deltaTime, GameState & /*gameState*/) override {
        position.y -= speed * static_cast<float>(deltaTime);
        if (position.y < -1.1f) {
            position.y = 1.1f;
            position.x = -1.0f + dist(gen) * 2.0f;
        }
        return false;
    }

    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override {
        glColor4f(brightness, brightness, brightness, brightness * 0.8f);
        glPointSize(size);
        glBegin(GL_POINTS);
        glVertex3f(position.x - cameraOffset.x, position.y - cameraOffset.y, -0.99f);
        glEnd();
    }
};

struct Background : Drawable, Updatable {
    std::vector<Star> stars;
    int lastUpdateTime;

    Background() : lastUpdateTime(0) { initializeStars(); }
    ~Background() override {}

    void initializeStars() {
        const int NUM_STARS = 60;
        stars.reserve(NUM_STARS);
        for (int i = 0; i < NUM_STARS; ++i) {
            float x = -1.0f + dist(gen) * 2.0f;
            float y = -1.0f + dist(gen) * 2.0f;
            float speed = 0.0008f + dist(gen) * 0.001f;
            float size = 1.0f + dist(gen) * 3.0f;
            float brightness = 0.3f + dist(gen) * 0.7f;
            stars.emplace_back(glm::fvec2(x, y), speed, size, brightness);
        }
    }

    bool update(int currentTime, GameState &gameState) override {
        if (lastUpdateTime == 0) {
            lastUpdateTime = currentTime;
            return false;
        }

        int deltaTime = currentTime - lastUpdateTime;
        lastUpdateTime = currentTime;

        for (auto &star : stars) {
            star.update(deltaTime, gameState);
        }
        return false;
    }

    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_POINT_SMOOTH);

        for (auto &star : stars) {
            star.draw(cameraOffset, gameState);
        }

        glDisable(GL_POINT_SMOOTH);
        glDisable(GL_BLEND);
    }
};

struct GameState {
    GameState(int h, int bh)
        : MAX_PLAYER_HEALTH(h), MAX_BOSS_HEALTH(bh), playerHealth(h), bossHealth(bh),
          cameraOffset(0.0f, 0.0f), playerObject(glm::fvec2(0.0f, -0.8f)),
          bossObject(glm::fvec2(0.0f, 0.6f)), bossHealthBarObject(glm::fvec2(0.0f, 0.0f)),
          heartsObject(glm::fvec2(0.0f, 0.0f)) {}

    int MAX_PLAYER_HEALTH;
    const int MAX_BOSS_HEALTH;
    int playerHealth;
    int bossHealth;
    glm::fvec2 cameraOffset;

    Player playerObject;
    Boss bossObject;
    BossHealthBar bossHealthBarObject;
    PlayerHealthBar heartsObject;
    Background backgroundObject;

    std::vector<PlayerBullet> playerBulletObjects;
    std::vector<EnemyBullet> enemyBulletObjects;
};

struct CommandExecutor : Updatable {
    std::vector<char> commandSequence;
    std::size_t currentIndex = 0;
    bool previousKeyStates[256] = {false};
    bool commandUsed = false;
    std::function<void(GameState &)> onActivate;

    CommandExecutor(std::vector<char> sequence, std::function<void(GameState &)> activateFunc)
        : commandSequence(std::move(sequence)), onActivate(std::move(activateFunc)) {}
    ~CommandExecutor() override = default;

    bool update(int currentTime, GameState &gameState) override {
        // Check for key press events (rising edge detection)
        for (int i = 0; i < 256; i++) {
            if (keyStates[i] && !previousKeyStates[i]) {
                // Key was just pressed
                handleKeyPress(static_cast<char>(i), gameState);
            }
            previousKeyStates[i] = keyStates[i];
        }
        return false;
    }

    void handleKeyPress(char key, GameState &gameState) {
        if (commandUsed)
            return;

        // Check if the pressed key matches the current position in the sequence
        if (currentIndex < commandSequence.size() && key == commandSequence[currentIndex]) {
            currentIndex++;

            // Check if the entire sequence has been completed
            if (currentIndex >= commandSequence.size()) {
                activateCommand(gameState);
            }
        } else {
            // Reset if wrong key pressed, but check if this key could start the sequence
            if (!commandSequence.empty() && key == commandSequence[0]) {
                currentIndex = 1;
            } else {
                currentIndex = 0;
            }
        }
    }

    void activateCommand(GameState &gameState) {
        if (commandUsed)
            return;

        commandUsed = true;
        onActivate(gameState);
    }
};

bool Player::update(int currentTime, GameState &gameState) {
    if (isDying) {
        // Update fragments
        int deltaTime = currentTime - deathStartTime;
        std::erase_if(fragments, [deltaTime, &gameState](auto &fragment) {
            return fragment.update(deltaTime, gameState);
        });

        // Exit game after 4 seconds
        if ((currentTime - deathStartTime) > 4000) {
            std::cout << "Game Over! Exiting...\n";
            std::exit(0);
        }

        return false;
    }

    // Check if player should die
    if (gameState.playerHealth <= 0 && !isDying) {
        startDeathAnimation(currentTime);
        startCameraShake(currentTime);
        return false;
    }

    // Check if invincibility period has ended
    if (isInvincible && currentTime >= invincibilityEndTime) {
        isInvincible = false;
    }

    float tiltSpeed = 0.35f;
    if (std::abs(targetTiltAngle - tiltAngle) > 0.1f) {
        tiltAngle += (targetTiltAngle - tiltAngle) * tiltSpeed;
    } else {
        tiltAngle = targetTiltAngle;
    }

    if (currentTime >= this->coolTime && this->isBullet) {
        // Fire bullets dynamically based on bulletCount
        float spacing = 0.03f; // Base spacing between bullets
        float totalWidth = spacing * static_cast<float>(bulletCount - 1);
        float startX = -totalWidth / 2.0f;
        float yForwardOffset = 0.04f; // How far forward the center bullet is

        for (int i = 0; i < bulletCount; ++i) {
            float xOffset = startX + (spacing * static_cast<float>(i));
            // Calculate Y offset - center bullets are more forward
            float centerDistance =
                std::abs(static_cast<float>(i) - static_cast<float>(bulletCount - 1) / 2.0f);
            float normalizedDistance =
                centerDistance / (static_cast<float>(bulletCount - 1) / 2.0f);
            float yOffset = yForwardOffset * (1.0f - normalizedDistance);

            gameState.playerBulletObjects.emplace_back(
                this->currentPosition + glm::fvec2(xOffset, yOffset), 0.003f, currentTime);
        }

        this->isBullet = false;
        this->coolTime = currentTime + 100;
    }
    return false;
}

bool EnemyBullet::update(int currentTime, GameState &gameState) {
    int dt = currentTime - initialTime;
    currentPosition =
        initialPosition + float(dt) * initialDirection + posFunc(dt, speed) * normalDirection;
    // Don't damage player if boss is already dying
    if (!gameState.bossObject.isDying && !gameState.playerObject.isInvincible &&
        detectCollision(*this, gameState.playerObject)) {
        gameState.playerHealth -= 0;
        gameState.playerObject.takeDamage(currentTime);
        startCameraShake(currentTime);
        if (gameState.playerHealth < 0)
            gameState.playerHealth = 0;
        return true;
    }
    return abs(currentPosition.x) > 1.0f || abs(currentPosition.y) > 1.0f;
}

bool PlayerBullet::update(int currentTime, GameState &gameState) {
    currentPosition =
        initialPosition + glm::fvec2(0, speed * static_cast<float>(currentTime - initialTime));
    // Don't damage boss if player is already dying
    if (!gameState.playerObject.isDying && detectCollision(*this, gameState.bossObject)) {
        gameState.bossHealth -= 1;
        if (gameState.bossHealth < 0)
            gameState.bossHealth = 0;
        return true;
    }
    return abs(currentPosition.x) > 1.0f || abs(currentPosition.y) > 1.0f;
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
    constexpr int BULLET_COUNT = 20;
    bullets.reserve(BULLET_COUNT);
    constexpr float SPEED = 0.0003f;
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
    constexpr int BULLET_COUNT = 8;
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
    constexpr int BULLET_COUNT = 5;
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
    if (isDying) {
        // Update fragments
        int deltaTime = currentTime - deathStartTime;
        std::erase_if(fragments, [deltaTime, &gameState](auto &fragment) {
            return fragment.update(deltaTime, gameState);
        });

        // Exit game after 5 seconds
        if ((currentTime - deathStartTime) > 5000) {
            std::cout << "Boss defeated! Exiting game...\n";
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

    // Draw background bar with purple tint
    glBegin(GL_QUADS);
    glColor4f(0.15f, 0.05f, 0.2f, 0.8f);
    glVertex3f(-barWidth / 2, barY - barHeight / 2, zDepth);
    glVertex3f(barWidth / 2, barY - barHeight / 2, zDepth);
    glVertex3f(barWidth / 2, barY + barHeight / 2, zDepth);
    glVertex3f(-barWidth / 2, barY + barHeight / 2, zDepth);
    glEnd();

    // Draw health bar with purple gradient
    float healthBarWidth = barWidth * healthPercentage;
    float healthBarX = -barWidth / 2 + healthBarWidth / 2;

    glm::fvec4 healthColor;

    if (healthPercentage > 0.5f) {
        healthColor = glm::fvec4(0.6f, 0.2f, 1.0f, 1.0f); // Bright purple
    } else if (healthPercentage > 0.25f) {
        healthColor = glm::fvec4(0.8f, 0.3f, 0.8f, 1.0f); // Pink-purple
    } else {
        healthColor = glm::fvec4(1.0f, 0.2f, 0.6f, 1.0f); // Red-purple (critical)
    }

    if (gameState.bossHealth > 0) {
        drawRectWithGlow(healthBarX, barY, healthBarWidth, barHeight, healthColor, 0.05f, 1.0f);
    }

    glDisable(GL_BLEND);

    glPopMatrix();
}

GameState gameState(5, 1000);

// Konami Command: up, up, down, down, left, right, left, right, B, A
// This command is widely known in gaming culture for granting special
CommandExecutor commandExecutor({'w', 'w', 's', 's', 'a', 'd', 'a', 'd', 'b', 'a'},
                                [](GameState &gameState) {
                                    // Activate Konami command effects
                                    gameState.MAX_PLAYER_HEALTH = 10;
                                    gameState.playerHealth = 10;

                                    // Grant 5 seconds of invincibility
                                    int currentTime = glutGet(GLUT_ELAPSED_TIME);
                                    gameState.playerObject.isInvincible = true;
                                    gameState.playerObject.invincibilityEndTime =
                                        currentTime + 5000; // 5 seconds

                                    // Upgrade to 5 bullets
                                    gameState.playerObject.bulletCount = 5;

                                    std::cout << "Konami Command Activated! Power up!" << '\n';
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

    gameState.backgroundObject.update(now, gameState);
    gameState.playerObject.update(now, gameState);
    gameState.bossObject.update(now, gameState);
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
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}
