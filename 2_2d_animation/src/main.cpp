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
#include <random>
#include <iomanip>
#include "collision.hpp"
#include "utils.hpp"

struct GameState;
bool keyStates[256] = {false};
struct BossMove;
void showVictoryScreen(const GameState &gameState);

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

struct TrailParticle : Drawable, Updatable {
    glm::fvec2 position;
    glm::fvec2 velocity;
    float size;
    float alpha;
    glm::fvec3 color;
    int birthTime;

    TrailParticle(glm::fvec2 pos, glm::fvec2 vel, float sz, glm::fvec3 col, int currentTime)
        : position(pos), velocity(vel), size(sz), alpha(0.8f), color(col), birthTime(currentTime) {}

    bool update(int currentTime, GameState &) override {
        int deltaTime = currentTime - birthTime;
        float dt = static_cast<float>(deltaTime) * 0.001f;

        // Slower drift and fade
        position += velocity * dt * 0.3f;
        alpha -= dt * 0.5f;
        size *= (1.0f - dt * 0.2f);

        return alpha <= 0.0f || size <= 0.001f || deltaTime > 2000; // Last up to 2 seconds
    }

    void draw(glm::fvec2 cameraOffset, const GameState &) override {
        const glm::fvec2 VIEW_POS = position - cameraOffset;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glPushMatrix();
        glTranslatef(VIEW_POS.x, VIEW_POS.y, -0.1f);
        glScalef(size, size, 1.0f);

        // Draw a glowing circle with brighter center
        glBegin(GL_TRIANGLE_FAN);
        // Much brighter center (almost white)
        glColor4f(glm::min(color.r * 1.5f, 1.0f), glm::min(color.g * 1.5f, 1.0f),
                  glm::min(color.b * 1.5f, 1.0f), alpha * 1.2f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        // Fade to darker edges
        glColor4f(color.r * 0.3f, color.g * 0.3f, color.b * 0.3f, 0.0f);
        const int N = 8;
        for (int i = 0; i <= N; ++i) {
            float angle =
                static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(N);
            glVertex3f(std::cos(angle), std::sin(angle), 0.0f);
        }
        glEnd();

        glPopMatrix();
        glDisable(GL_BLEND);
    }
};

struct EnemyBullet : Updatable, Drawable, Collidable {
    glm::fvec2 initialDirection;
    glm::fvec2 normalDirection;
    glm::fvec2 initialPosition;
    glm::fvec2 currentPosition;
    glm::fvec2 previousPosition;
    int initialTime;
    int lastTrailTime;
    float speed;
    std::function<float(int, float)> posFunc;

    EnemyBullet(glm::fvec2 initialDirection, glm::fvec2 initialPosition, float speed,
                int initialTime, std::function<float(int, float)> posFunc)
        : initialDirection(glm::normalize(initialDirection) * speed),
          normalDirection(glm::normalize(glm::fvec2(-initialDirection.y, initialDirection.x))),
          initialPosition(initialPosition), currentPosition(initialPosition),
          previousPosition(initialPosition), initialTime(initialTime), lastTrailTime(initialTime),
          speed(speed), posFunc(std::move(posFunc)) {}
    ~EnemyBullet() override {}

    bool update(int currentTime, GameState &gameState) override;
    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override {
        const glm::fvec2 VIEW_POS = currentPosition - cameraOffset;

        const float BASE_SCALE = 0.03f;

        const float T_MS = static_cast<float>(glutGet(GLUT_ELAPSED_TIME));
        const float SPOKES_ROTATION_DEG = T_MS * 0.005f * 180.0f / std::numbers::pi_v<float>;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glPushMatrix();
        glTranslatef(VIEW_POS.x, VIEW_POS.y, 0.0f);
        glScalef(BASE_SCALE, BASE_SCALE, 1.0f);

        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.8f, 0.2f, 1.0f, 0.3f);
        glVertex3f(0.0f, 0.0f, -0.05f);
        glColor4f(0.4f, 0.0f, 0.8f, 0.0f);
        const int N = 12;
        const float OUTER_R = 2.0f;
        for (int i = 0; i <= N; ++i) {
            float ang = (float)i * glm::two_pi<float>() / (float)N;
            glVertex3f(OUTER_R * std::cos(ang), OUTER_R * std::sin(ang), -0.05f);
        }
        glEnd();

        glBegin(GL_QUADS);
        glColor4f(1.0f, 0.3f, 0.8f, 1.0f);
        glVertex3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.7f, 0.0f, 0.0f);
        glVertex3f(0.0f, -1.0f, 0.0f);
        glVertex3f(-0.7f, 0.0f, 0.0f);
        glEnd();

        glBegin(GL_QUADS);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glVertex3f(0.0f, 0.5f, 0.01f);
        glVertex3f(0.35f, 0.0f, 0.01f);
        glVertex3f(0.0f, -0.5f, 0.01f);
        glVertex3f(-0.35f, 0.0f, 0.01f);
        glEnd();

        glPushMatrix();
        glRotatef(SPOKES_ROTATION_DEG, 0.0f, 0.0f, 1.0f);

        glLineWidth(1.5f);
        glBegin(GL_LINES);
        glColor4f(0.6f, 0.1f, 1.0f, 0.7f);
        const float SPOKES_R = 1.2f;
        for (int i = 0; i < 4; ++i) {
            float ang = (float)i * glm::half_pi<float>();
            glVertex3f(0.0f, 0.0f, 0.02f);
            glVertex3f(SPOKES_R * std::cos(ang), SPOKES_R * std::sin(ang), 0.02f);
        }
        glEnd();

        glPopMatrix();
        glPopMatrix();

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
        const float WIDTH = 0.015f;
        const float HEIGHT = 0.04f;
        const float Z_DEPTH = 0.0f;

        const glm::fvec2 WORLD_POS = currentPosition;
        const glm::fvec2 VIEW_POS = WORLD_POS - cameraOffset;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glPushMatrix();
        glTranslatef(VIEW_POS.x, VIEW_POS.y, 0.0f);
        glRotatef(0.0f, 0.0f, 0.0f, 1.0f);
        glScalef(WIDTH, HEIGHT, 1.0f);

        glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.9f, 0.0f, 1.0f);
        glVertex3f(0.0f, 1.0f, Z_DEPTH);
        glColor4f(1.0f, 0.5f, 0.0f, 0.8f);
        glVertex3f(-1.0f, -0.3f, Z_DEPTH);
        glVertex3f(1.0f, -0.3f, Z_DEPTH);
        glEnd();

        glBegin(GL_QUADS);
        glColor4f(1.0f, 0.7f, 0.0f, 1.0f);
        glVertex3f(-0.6f, -0.2f, Z_DEPTH);
        glVertex3f(0.6f, -0.2f, Z_DEPTH);
        glColor4f(1.0f, 0.3f, 0.0f, 0.2f);
        glVertex3f(0.4f, -1.5f, Z_DEPTH);
        glVertex3f(-0.4f, -1.5f, Z_DEPTH);
        glEnd();

        glPointSize(8.0f);
        glBegin(GL_POINTS);
        glColor4f(1.0f, 1.0f, 0.7f, 0.9f);
        glVertex3f(0.0f, 0.7f, Z_DEPTH);
        glEnd();

        glPopMatrix();

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
        const glm::fvec2 VIEW_POS = position - cameraOffset;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glPushMatrix();
        glTranslatef(VIEW_POS.x, VIEW_POS.y, 0.0f);
        glRotatef(rotation, 0.0f, 0.0f, 1.0f);
        glScalef(size, size, 1.0f);

        const float H = std::sqrt(3.0f) / 2.0f;

        glBegin(GL_TRIANGLES);
        glColor4f(color.r, color.g, color.b, alpha);
        glVertex3f(0.0f, 1.0f, 0.0f);

        glColor4f(color.r * 0.5f, color.g * 0.5f, color.b * 0.5f, alpha * 0.5f);
        glVertex3f(-H, -0.5f, 0.0f);
        glVertex3f(H, -0.5f, 0.0f);
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
            // 파편은 그대로(파편 내부에서 동일한 방식으로 모델행렬 쓰는 게 이상적)
            for (auto &fragment : fragments) {
                fragment.draw(cameraOffset, gameState);
            }

            // 폭발 효과: 원점 단위 원을 그리고 모델 행렬로 위치/스케일 적용
            int currentTime = glutGet(GLUT_ELAPSED_TIME);
            float timeSinceDeath = static_cast<float>(currentTime - deathStartTime) * 0.001f;

            if (timeSinceDeath < 1.2f) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);

                float explosionSize = 0.2f * (1.0f + timeSinceDeath * 3.0f);
                float alpha = 1.0f - timeSinceDeath * 0.83f;

                // M = T(pos - camera) * S(explosionSize)
                glm::mat4 m = glm::translate(glm::mat4(1.0f),
                                             glm::vec3(currentPosition - cameraOffset, 0.0f));
                m = glm::scale(m, glm::vec3(explosionSize, explosionSize, 1.0f));

                glPushMatrix();
                glMultMatrixf(glm::value_ptr(m));

                glBegin(GL_TRIANGLE_FAN);
                glColor4f(1.0f, 0.9f, 0.0f, alpha * 0.8f);
                glVertex3f(0.0f, 0.0f, 0.0f); // 중심(로컬 원점)

                glColor4f(1.0f, 0.5f, 0.0f, 0.0f);
                for (int i = 0; i <= 20; ++i) {
                    float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / 20.0f;
                    // 로컬 단위 원 좌표, 변환은 모델 행렬이 담당
                    glVertex3f(std::cos(angle), std::sin(angle), 0.0f);
                }
                glEnd();

                glPopMatrix();
                glDisable(GL_BLEND);
            }
            return;
        }

        // 우주선: drawSpaceship을 원점/단위 스케일 기준으로 호출하고,
        // 모델 행렬로 위치/회전/스케일을 적용
        // M = T(current - camera) * R_y(tiltAngle) * S(0.14)
        glm::mat4 m =
            glm::translate(glm::mat4(1.0f), glm::vec3(currentPosition - cameraOffset, 0.0f));
        m = glm::rotate(m, glm::radians(tiltAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::scale(m, glm::vec3(0.14f, 0.14f, 0.14f));

        glPushMatrix();
        glMultMatrixf(glm::value_ptr(m));

        if (isInvincible) {
            float alpha =
                0.3f +
                0.4f * std::abs(std::sin(static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.01f));
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // 로컬 기준으로 그리기 (원점, 단위 스케일)
            drawSpaceship(glm::fvec2(0.0f, 0.0f), 1.0f, glm::fvec4(1.0f, 1.0f, 0.0f, alpha));
            glDisable(GL_BLEND);
        } else {
            drawSpaceship(glm::fvec2(0.0f, 0.0f), 1.0f, glm::fvec4(1.0f, 1.0f, 0.0f, 1.0f));
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
        if (glm::length(directionVector) < 1e-2)
            normalVector = glm::fvec2(0, 0);
        else
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
        const glm::fvec2 VIEW_POS = position - cameraOffset;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 기존 가산 블렌딩 유지

        glPushMatrix();
        glTranslatef(VIEW_POS.x, VIEW_POS.y, 0.0f); // 위치
        glRotatef(rotation, 0.0f, 0.0f, 1.0f);      // 회전
        glScalef(size, size, 1.0f);                 // 크기 (행렬로 처리)

        // 원점 기준 단위 정삼각형: (0,1), (-√3/2,-1/2), (√3/2,-1/2)
        constexpr float H = 0.8660254f; // √3/2

        glBegin(GL_TRIANGLES);
        glColor4f(color.r, color.g, color.b, alpha);
        glVertex3f(0.0f, 1.0f, 0.0f);

        glColor4f(color.r * 0.5f, color.g * 0.5f, color.b * 0.5f, alpha * 0.5f);
        glVertex3f(-H, -0.5f, 0.0f);
        glVertex3f(H, -0.5f, 0.0f);
        glEnd();

        glPopMatrix();
        glDisable(GL_BLEND);
    }
};

struct BossArm : Drawable {
    // 4개 관절 각도
    float angles[4] = {0.0f}; // 어깨, 상완, 하완, 손목

    // 세그먼트 길이
    float lengths[4] = {0.8f, 0.7f, 0.6f, 0.4f};
    float armWidth = 0.12f;
    glm::vec3 armColor = glm::vec3(0.7f, 0.2f, 0.9f);

    // 각 보스마다 다른 특성
    int bossId; // 1 또는 2
    bool isLeftArm;
    float uniquePhase[4]; // 각 보스의 고유한 위상
    float uniqueSpeed[4]; // 각 보스의 고유한 속도
    float baseDirection;  // 기본 방향

    BossArm(bool isLeft, int boss)
        : isLeftArm(isLeft), bossId(boss), uniquePhase{0.0f, 0.0f, 0.0f, 0.0f},
          uniqueSpeed{0.0f, 0.0f, 0.0f, 0.0f} {
        std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * std::numbers::pi_v<float>);
        std::uniform_real_distribution<float> speedDist(0.3f, 1.2f);
        std::uniform_real_distribution<float> dirDist(-60.0f, 60.0f);

        // 각 보스와 팔마다 고유한 특성 생성
        float bossMultiplier = (bossId == 1) ? 1.0f : 1.5f; // 보스2가 더 빠름
        float armMultiplier = isLeftArm ? 1.0f : 1.2f;      // 좌우 팔 다름

        for (int i = 0; i < 4; ++i) {
            uniquePhase[i] = phaseDist(gen) + (static_cast<float>(bossId) * 1.2f) +
                             (isLeftArm ? 0.0f : 2.1f) + (static_cast<float>(i) * 0.7f);
            uniqueSpeed[i] = (0.4f + speedDist(gen) * 0.6f) * bossMultiplier * armMultiplier;
            angles[i] = 0.0f;
        }

        // 방향성을 랜덤하게
        baseDirection = dirDist(gen);
    }

    void update(float time) {
        // 각 관절의 연속적인 움직임 계산
        float intensities[4] = {35.0f, 20.0f, 15.0f, 10.0f}; // 관절별 최대 각도
        float limits[4][2] = {{-70.0f, 70.0f}, {-40.0f, 40.0f}, {-30.0f, 30.0f}, {-20.0f, 20.0f}};

        for (int i = 0; i < 4; ++i) {
            // 연속적인 사인파 기반 움직임
            float mainWave = std::sin(time * uniqueSpeed[i] + uniquePhase[i]);
            float microWave = std::sin(time * uniqueSpeed[i] * 2.3f + uniquePhase[i] + 1.0f) * 0.3f;

            // 기본 각도 계산
            float targetAngle = (mainWave + microWave) * intensities[i];

            // 어깨는 기본적으로 바깥쪽으로 펼쳐지게 하기
            if (i == 0) {
                float armSign = isLeftArm ? -1.0f : 1.0f;
                targetAngle = targetAngle * 0.6f + armSign * 40.0f + baseDirection * 0.3f;
            }

            // 각도 제한
            angles[i] = glm::clamp(targetAngle, limits[i][0], limits[i][1]);
        }
    }

    void drawSegment(float length, float width, float brightness) {
        glBegin(GL_QUADS);
        glColor4f(armColor.r * brightness, armColor.g * brightness, armColor.b * brightness, 1.0f);
        glVertex3f(-width / 2, 0.0f, 0.0f);
        glVertex3f(width / 2, 0.0f, 0.0f);
        glColor4f(armColor.r * brightness * 0.7f, armColor.g * brightness * 0.7f,
                  armColor.b * brightness * 0.7f, 1.0f);
        glVertex3f(width / 2, -length, 0.0f);
        glVertex3f(-width / 2, -length, 0.0f);
        glEnd();
    }

    void drawJoint(float size) {
        glPushMatrix();
        glScalef(size, size, 1.0f);
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(armColor.r * 1.2f, armColor.g * 1.2f, armColor.b * 1.2f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.01f);
        glColor4f(armColor.r * 0.6f, armColor.g * 0.6f, armColor.b * 0.6f, 0.8f);
        for (int i = 0; i <= 8; ++i) {
            float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / 8.0f;
            glVertex3f(std::cos(angle), std::sin(angle), 0.01f);
        }
        glEnd();
        glPopMatrix();
    }

    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override {
        float t = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.001f;
        update(t);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float shoulderX = isLeftArm ? -1.0f : 1.0f;
        float shoulderY = 0.1f;

        glPushMatrix();
        glTranslatef(shoulderX, shoulderY, 0.01f);

        // 4개 관절을 순서대로 그리기
        for (int i = 0; i < 4; ++i) {
            glRotatef(angles[i], 0.0f, 0.0f, 1.0f);
            drawJoint(armWidth * (1.6f - float(i) * 0.2f));
            drawSegment(lengths[i], armWidth, 1.2f - float(i) * 0.1f);
            glTranslatef(0.0f, -lengths[i], 0.0f);
        }

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
    BossArm leftArm{true, 1};
    BossArm rightArm{false, 1};

    Boss(glm::fvec2 initialPosition, int id = 1)
        : currentPosition(initialPosition), currentMove(idleBossMove(initialPosition)),
          leftArm(true, id), rightArm(false, id) {}
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
    static void drawUnitCircleFan(int seg, float z, const glm::vec4 &centerRGBA,
                                  const glm::vec4 &edgeRGBA) {
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(centerRGBA.r, centerRGBA.g, centerRGBA.b, centerRGBA.a);
        glVertex3f(0.f, 0.f, z);
        glColor4f(edgeRGBA.r, edgeRGBA.g, edgeRGBA.b, edgeRGBA.a);
        for (int i = 0; i <= seg; ++i) {
            float ang = (float)i * 2.f * std::numbers::pi_v<float> / (float)seg;
            glVertex3f(std::cos(ang), std::sin(ang), z);
        }
        glEnd();
    }

    // 반지름=1 정팔각형 팬 (중심 포함)
    static void drawUnitOctagonFan(float z, const glm::vec4 &centerRGBA,
                                   const glm::vec4 &edgeRGBA) {
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(centerRGBA.r, centerRGBA.g, centerRGBA.b, centerRGBA.a);
        glVertex3f(0.f, 0.f, z);
        glColor4f(edgeRGBA.r, edgeRGBA.g, edgeRGBA.b, edgeRGBA.a);
        for (int i = 0; i <= 8; ++i) {
            float ang = (float)i * std::numbers::pi_v<float> / 4.f;
            glVertex3f(std::cos(ang), std::sin(ang), z);
        }
        glEnd();
    }

    // 스파이크 1개 (원점에서 시작, 외곽·내곽 반지름을 유닛으로 받음)
    static void drawUnitSpikeTri(float rOuter, float rInner, float z, const glm::vec4 &innerRGBA,
                                 const glm::vec4 &tipRGBA) {
        glBegin(GL_TRIANGLES);
        glColor4f(innerRGBA.r, innerRGBA.g, innerRGBA.b, innerRGBA.a);
        glVertex3f(0.f, 0.f, z);
        glColor4f(tipRGBA.r, tipRGBA.g, tipRGBA.b, tipRGBA.a);
        glVertex3f(rOuter, 0.f, z);
        glVertex3f(rInner * std::cos(0.2f), rInner * std::sin(0.2f), z);
        glEnd();
    }

    // 메인 드로우 ------------------------------------------------------
    void draw(glm::fvec2 cameraOffset, const GameState &gameState) override {
        const glm::fvec2 VIEW_POS = currentPosition - cameraOffset;
        const float SIZE = 0.15f; // 전체 스케일(월드 단위)
        const float T = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.001f;

        if (isDying) {
            // 파편
            for (auto &fragment : fragments) {
                fragment.draw(cameraOffset, gameState);
            }

            // 폭발(행렬 기반)
            const int NOW_MS = glutGet(GLUT_ELAPSED_TIME);
            const float DT = static_cast<float>(NOW_MS - deathStartTime) * 0.001f;
            if (DT < 1.5f) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 가산 혼합 유지

                const float EXPLOSION_SIZE = 0.3f * (1.0f + DT * 2.0f); // 느리게 팽창
                const float A = 1.0f - DT * 0.67f;                      // 느리게 페이드

                glPushMatrix();
                glTranslatef(VIEW_POS.x, VIEW_POS.y, 0.f);
                glScalef(EXPLOSION_SIZE, EXPLOSION_SIZE, 1.f);
                drawUnitCircleFan(
                    /*seg=*/20, /*z=*/0.0f,
                    /*center*/ glm::vec4(1.0f, 0.8f, 1.0f, A * 0.8f),
                    /*edge  */ glm::vec4(0.8f, 0.2f, 1.0f, 0.0f));
                glPopMatrix();

                glDisable(GL_BLEND);
            }
            return;
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 본체는 일반 알파 블렌딩

        // ===== 모델 행렬: 위치 → (필요시 회전) → 스케일 =====
        glPushMatrix();
        glTranslatef(VIEW_POS.x, VIEW_POS.y, 0.f);
        // 필요하다면 전체 회전이 있으면 여기서 glRotatef(angle, 0,0,1) 추가
        glScalef(SIZE, SIZE, 1.f);

        // 1) 본체(정팔각형)
        drawUnitOctagonFan(
            /*z=*/0.0f,
            /*center*/ glm::vec4(0.3f, 0.0f, 0.8f, 1.0f),
            /*edge  */ glm::vec4(0.1f, 0.0f, 0.6f, 1.0f));

        // 2) 코어 글로우 (반지름 0.6)
        glPushMatrix();
        glScalef(0.6f, 0.6f, 1.f);
        drawUnitCircleFan(
            /*seg=*/20, /*z=*/0.01f,
            /*center*/ glm::vec4(0.8f, 0.2f, 1.0f, 0.8f),
            /*edge  */ glm::vec4(0.4f, 0.0f, 0.8f, 0.2f));
        glPopMatrix();

        // 3) 회전 스파이크 (6개) — 자식 행렬로 회전만 적용
        glPushMatrix();
        glRotatef(T * 57.2957795f, 0.f, 0.f, 1.f); // rad→deg
        for (int i = 0; i < 6; ++i) {
            glPushMatrix();
            glRotatef((360.f / 6.f) * static_cast<float>(i), 0.f, 0.f, 1.f);
            drawUnitSpikeTri(
                /*rOuter=*/1.3f, /*rInner=*/0.8f, /*z=*/0.02f,
                /*innerRGBA*/ glm::vec4(0.6f, 0.1f, 1.0f, 0.9f),
                /*tipRGBA  */ glm::vec4(0.2f, 0.0f, 0.4f, 0.6f));
            glPopMatrix();
        }
        glPopMatrix();

        // 4) 아이/코어 디테일 (반지름 0.3)
        glPushMatrix();
        glScalef(0.3f, 0.3f, 1.f);
        drawUnitCircleFan(
            /*seg=*/10, /*z=*/0.03f,
            /*center*/ glm::vec4(1.0f, 0.0f, 0.5f, 1.0f),
            /*edge  */ glm::vec4(0.3f, 0.0f, 0.2f, 1.0f));
        glPopMatrix();

        // 5) 에너지 필드 (선 루프, 반지름 1.1 + 요동)
        {
            const int SEG = 16;
            // 원래 코드의 wobble(절대 0.02)을 유지하려면 유닛 공간에서는 0.02/size
            const float WOBBLE_UNIT = 0.02f / SIZE;

            glLineWidth(2.0f); // glBegin 밖에서 설정
            glBegin(GL_LINE_LOOP);
            glColor4f(0.4f, 0.2f, 1.0f, 0.5f);
            for (int i = 0; i < SEG; ++i) {
                float ang = (float)i * 2.f * std::numbers::pi_v<float> / (float)SEG;
                float wobble = std::sin(T * 3.0f + ang * 2.0f) * WOBBLE_UNIT;
                float r = 1.1f + wobble; // 유닛 반지름
                glVertex3f(r * std::cos(ang), r * std::sin(ang), 0.0f);
            }
            glEnd();
        }

        // 6) 좌우 팔들 (보스 좌표계에서 그리기 - 보스 matrix stack 내부에서)
        leftArm.draw(cameraOffset, gameState);
        rightArm.draw(cameraOffset, gameState);

        glPopMatrix(); // 모델 행렬 끝
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

    void draw(glm::fvec2 cameraOffset, const GameState &) override {
        const glm::fvec2 VIEW_POS = position - cameraOffset;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glColor4f(brightness, brightness, brightness, brightness * 0.8f);
        glPointSize(size);

        glPushMatrix();
        glTranslatef(VIEW_POS.x, VIEW_POS.y, -0.99f);

        glBegin(GL_POINTS);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glEnd();

        glPopMatrix();
        glDisable(GL_BLEND);
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
          bossObject1(glm::fvec2(0.5f, 0.6f), 1), bossObject2(glm::fvec2(-0.5f, 0.6f), 2),
          bossHealthBarObject(glm::fvec2(0.0f, 0.0f)), heartsObject(glm::fvec2(0.0f, 0.0f)) {}

    int MAX_PLAYER_HEALTH;
    const int MAX_BOSS_HEALTH;
    int playerHealth;
    int bossHealth;
    glm::fvec2 cameraOffset;
    bool konamiUsed = false;

    Player playerObject;
    Boss bossObject1;
    Boss bossObject2;
    BossHealthBar bossHealthBarObject;
    PlayerHealthBar heartsObject;
    Background backgroundObject;

    std::vector<PlayerBullet> playerBulletObjects;
    std::vector<EnemyBullet> enemyBulletObjects;
    std::vector<TrailParticle> trailParticles;
};

void showVictoryScreen(const GameState &gameState) {
    int elapsedTime = glutGet(GLUT_ELAPSED_TIME);
    int seconds = elapsedTime / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;

    std::cout << "\n\n";
    std::cout << "\033[1;36m"
              << "============================================================================\n";
    std::cout << "\033[1;33m"
              << "                                                                             \n";
    std::cout << "\033[1;33m"
              << "       ██╗   ██╗██╗ ██████╗████████╗ ██████╗ ██████╗ ██╗   ██╗██╗            \n";
    std::cout << "\033[1;33m"
              << "       ██║   ██║██║██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗╚██╗ ██╔╝██║            \n";
    std::cout << "\033[1;33m"
              << "       ██║   ██║██║██║        ██║   ██║   ██║██████╔╝ ╚████╔╝ ██║            \n";
    std::cout << "\033[1;33m"
              << "       ╚██╗ ██╔╝██║██║        ██║   ██║   ██║██╔══██╗  ╚██╔╝  ╚═╝            \n";
    std::cout << "\033[1;33m"
              << "        ╚████╔╝ ██║╚██████╗   ██║   ╚██████╔╝██║  ██║   ██║   ██╗            \n";
    std::cout << "\033[1;33m"
              << "         ╚═══╝  ╚═╝ ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚═╝            \n";
    std::cout << "\033[1;33m"
              << "                                                                             \n";
    if (gameState.konamiUsed) {
        std::cout
            << "\033[1;35m"
            << "                             ↑↑↓↓←→←→BA                                    \n";
    } else {
        std::cout << "\033[1;35m"
                  << "                          ⟡ BOSS DEFEATED ⟡                              \n";
    }
    std::cout << "\033[1;36m"
              << "============================================================================\n\n";

    std::cout << "\033[1;32m" << "                        ╔════════════════════╗\n";
    std::cout << "\033[1;32m" << "                        ║   GAME STATISTICS  ║\n";
    std::cout << "\033[1;32m" << "                        ╚════════════════════╝\n\n";

    std::cout << "\033[1;37m" << "                    ⏱  Clear Time: " << "\033[1;33m";
    std::cout << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0')
              << std::setw(2) << seconds << "\033[0m\n\n";

    std::cout << "\033[1;37m" << "                    ❤  Lives Remaining: " << "\033[1;31m";
    for (int i = 0; i < gameState.playerHealth; i++) {
        std::cout << "♥";
    }
    std::cout << " (" << gameState.playerHealth << "/" << gameState.MAX_PLAYER_HEALTH
              << ")\033[0m\n\n";

    std::cout << "\033[1;36m"
              << "============================================================================\n";
    std::cout << "\033[1;35m"
              << "                      Thank you for playing!                              \n";
    std::cout << "\033[1;36m"
              << "============================================================================\n";
    std::cout << "\033[0m\n\n";
}

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
    previousPosition = currentPosition;
    int dt = currentTime - initialTime;
    currentPosition =
        initialPosition + float(dt) * initialDirection + posFunc(dt, speed) * normalDirection;

    // Create trail particles
    if (currentTime - lastTrailTime > 15) { // Create particles every 15ms (more frequent)
        lastTrailTime = currentTime;

        // Create 2 particles per update for denser trail
        for (int i = 0; i < 2; ++i) {
            // Add more spread to the trail
            std::uniform_real_distribution<float> offsetDist(-0.015f, 0.015f);
            std::uniform_real_distribution<float> velDist(-0.025f, 0.025f);

            glm::fvec2 trailPos = currentPosition + glm::fvec2(offsetDist(gen), offsetDist(gen));
            glm::fvec2 trailVel(velDist(gen), velDist(gen));
            float trailSize = 0.025f + offsetDist(gen) * 0.3f; // Bigger particles

            // Slightly dimmed purple/pink trail color for better contrast
            glm::fvec3 trailColor(0.6f, 0.25f, 0.8f);

            gameState.trailParticles.emplace_back(trailPos, trailVel, trailSize, trailColor,
                                                  currentTime);
        }
    }

    // Don't damage player if boss is already dying
    if (!gameState.bossObject1.isDying && !gameState.playerObject.isInvincible &&
        detectCollision(*this, gameState.playerObject)) {
        gameState.playerHealth -= 1;
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
    if (!gameState.playerObject.isDying && (detectCollision(*this, gameState.bossObject1) ||
                                            detectCollision(*this, gameState.bossObject2))) {
        gameState.bossHealth -= 1;
        if (gameState.bossHealth < 0)
            gameState.bossHealth = 0;
        return true;
    }
    return abs(currentPosition.x) > 1.0f || abs(currentPosition.y) > 1.0f;
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
