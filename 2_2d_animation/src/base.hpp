#pragma once

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <functional>
#include <numbers>
#include <algorithm>
#include <optional>
#include <utility>
#include <vector>
#include <random>
#include <iomanip>
#include "collision.hpp"
#include "utils.hpp"

struct GameState;
struct EnemyBullet;
struct BossMove;

using BulletVec = std::vector<EnemyBullet>;
using BulletPattern = std::function<BulletVec(GameState &, int, int)>;
using PatternEntry = std::pair<BulletPattern, int>; // {패턴함수, 시작시각(ms)}

using MoveFn = std::function<BossMove(int, GameState &)>;
using MoveEntry = std::pair<MoveFn, int>;

std::optional<BossMove> getCurrentMove(int currentTime, GameState &gameState, int bossNum);

extern bool keyStates[256];
void startCameraShake(int currentTime);
BulletPattern getCurrentBulletPattern(int currentTime);

/// @brief Interface for objects that can be drawn
struct Drawable {
    /// @brief Draw the object with a given camera camera_offset
    /// @param camera_offset The camera offset to apply
    virtual void draw(const GameState &gameState) = 0;
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

struct TrailParticle : Drawable, Updatable {
    glm::fvec2 position;
    glm::fvec2 velocity;
    float size;
    float alpha;
    glm::fvec3 color;
    int birthTime;

    TrailParticle(glm::fvec2 pos, glm::fvec2 vel, float sz, glm::fvec3 col, int currentTime);
    bool update(int currentTime, GameState &) override;
    void draw(const GameState &) override;
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
                int initialTime, std::function<float(int, float)> posFunc);

    bool update(int currentTime, GameState &gameState) override;
    void draw(const GameState &gameState) override;
    CollisionShape getShape() const override;
};
struct PlayerBullet : Updatable, Drawable, Collidable {
    glm::fvec2 initialPosition;
    glm::fvec2 currentPosition;
    int initialTime;
    float speed;

    PlayerBullet(glm::fvec2 initialPosition, float speed, int initialTime);

    bool update(int currentTime, GameState &gameState) override;
    void draw(const GameState &gameState) override;
    CollisionShape getShape() const override;
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
                   glm::fvec3 col);

    bool update(int deltaTime, GameState &) override;
    void draw(const GameState &) override;
};

struct EnergyOrb : Drawable, Updatable {
    glm::fvec2 offset;         // 플레이어 중심으로부터의 오프셋
    float angle;               // 현재 회전 각도
    float orbitRadius;         // 궤도 반지름
    float size;                // 구체 크기
    int birthTime;             // 생성 시간
    glm::fvec2 playerPosition; // 플레이어 위치 저장

    EnergyOrb();
    EnergyOrb(float startAngle, float radius, float sz, int currentTime);

    void updatePosition();
    void setPlayerPosition(const glm::fvec2 &pos);
    bool update(int currentTime, GameState &) override;
    void draw(const GameState &) override;
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
    int bulletCount = 3;               // Number of bullets to fire at once
    std::vector<EnergyOrb> energyOrbs; // 에너지 구체들

    Player(glm::fvec2 initialPosition);

    void updateEnergyOrbs(int currentHealth, int currentTime);
    void startDeathAnimation(int currentTime);
    void tryAttack();
    bool update(int currentTime, GameState &gameState) override;
    void draw(const GameState &gameState) override;
    void move(glm::fvec2 deltaPosition);
    void takeDamage(int currentTime);
    CollisionShape getShape() const override;
};
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
    };
    glm::fvec2 getCurrentPosition(int currentTime);
};
BossMove idleBossMove(glm::fvec2 position, int startTime = 0);

struct BossFragment : Drawable, Updatable {
    glm::fvec2 position;
    glm::fvec2 velocity;
    float rotation;
    float rotationSpeed;
    float size;
    float alpha;
    glm::fvec3 color;

    BossFragment(glm::fvec2 pos, glm::fvec2 vel, float rot, float rotSpeed, float sz,
                 glm::fvec3 col);
    bool update(int deltaTime, GameState &) override;
    void draw(const GameState &) override;
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

    BossArm(bool isLeft, int boss);
    void update(float time);
    void drawSegment(float length, float width, float brightness);
    void drawJoint(float size);
    void draw(const GameState &gameState) override;
};
struct Boss : Updatable, Drawable, Collidable {
    glm::fvec2 currentPosition;
    BossMove currentMove;
    int bossId;
    int coolTime = 0;
    int coolTimePeriod = 500;
    bool isDying = false;
    int deathStartTime = 0;
    std::vector<BossFragment> fragments;
    BossArm leftArm{true, 1};
    BossArm rightArm{false, 1};

    Boss(glm::fvec2 initialPosition, int id = 1);

    void startDeathAnimation(int currentTime);
    bool update(int currentTime, GameState &gameState) override;
    static void drawUnitCircleFan(int seg, float z, const glm::vec4 &centerRGBA,
                                  const glm::vec4 &edgeRGBA);
    static void drawUnitOctagonFan(float z, const glm::vec4 &centerRGBA, const glm::vec4 &edgeRGBA);
    static void drawUnitSpikeTri(float rOuter, float rInner, float z, const glm::vec4 &innerRGBA,
                                 const glm::vec4 &tipRGBA);
    void draw(const GameState &gameState) override;
    CollisionShape getShape() const override;
};

struct PlayerHealthBar : Drawable {
    glm::fvec2 drawPosition;

    PlayerHealthBar(glm::fvec2 drawPosition);

    void draw(const GameState &gameState) override;
};
struct BossHealthBar : Drawable {
    glm::fvec2 drawPosition;

    BossHealthBar(glm::fvec2 drawPosition);

    void draw(const GameState &gameState) override;
};

struct Star : Drawable, Updatable {
    glm::fvec2 position;
    float speed;
    float size;
    float brightness;

    Star(glm::fvec2 pos, float spd, float sz, float br);
    bool update(int deltaTime, GameState & /*gameState*/) override;
    void draw(const GameState &) override;
};
struct Background : Drawable, Updatable {
    std::vector<Star> stars;
    int lastUpdateTime;

    Background();

    void initializeStars();
    bool update(int currentTime, GameState &gameState) override;
    void draw(const GameState &gameState) override;
};

struct GameState {
    GameState(int h, int bh);

    int MAX_PLAYER_HEALTH;
    const int MAX_BOSS_HEALTH;
    int playerHealth;
    int bossHealth;
    glm::fvec2 cameraShakeOffset;
    glm::fvec2 cameraBaseOffset;
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

void showVictoryScreen(const GameState &gameState);

struct CommandExecutor : Updatable {
    std::vector<char> commandSequence;
    std::size_t currentIndex = 0;
    bool previousKeyStates[256] = {false};
    bool commandUsed = false;
    std::function<void(GameState &)> onActivate;

    CommandExecutor(std::vector<char> sequence, std::function<void(GameState &)> activateFunc);
    bool update(int currentTime, GameState &gameState) override;
    void handleKeyPress(char key, GameState &gameState);
    void activateCommand(GameState &gameState);
};
