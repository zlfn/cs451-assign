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
#include <memory>
#include "collision.hpp"
#include "utils.hpp"
#include "Skybox.hpp"
#include "shaders/shaders.hpp"

struct GameState;
struct EnemyBullet;
struct BossMove;
struct LightSource;

extern MatrixStack modelViewStack;
extern MatrixStack projectionStack;

using BulletVec = std::vector<EnemyBullet>;
using BulletPattern = std::function<BulletVec(GameState &, int, int)>;
using PatternEntry = std::pair<BulletPattern, int>; // {패턴함수, 시작시각(ms)}

using MoveFn = std::function<BossMove(int, GameState &)>;
using MoveEntry = std::pair<MoveFn, int>;

std::optional<BossMove> getCurrentMove(int currentTime, GameState &gameState, int bossNum);

extern bool keyStates[256];
void startCameraShake(int currentTime);
BulletPattern getCurrentBulletPattern(int currentTime);

// 그릴 수 있는 객체의 인터페이스
struct Drawable {
    virtual void draw(const GameState &gameState) = 0;
    virtual ~Drawable() = default;
};

// 업데이트 가능한 객체의 인터페이스
struct Updatable {
    // 객체의 상태를 업데이트. 제거해야 할 경우 true 반환
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
    std::unique_ptr<Mesh> mesh;

    TrailParticle(glm::fvec2 pos, glm::fvec2 vel, float sz, glm::fvec3 col, int currentTime);
    bool update(int currentTime, GameState &) override;
    void draw(const GameState &) override;
private:
    void createMesh();
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

struct EnergyOrb : Drawable, Updatable {
    glm::fvec2 offset;
    float angle;
    float orbitRadius;
    float size;
    int birthTime;

    EnergyOrb(float startAngle, float radius, float sz, int currentTime);
    void updatePosition();
    bool update(int currentTime, GameState &) override;
    void draw(const GameState &) override;
};
struct PlayerFragment : Drawable, Updatable {
    glm::fvec2 position;
    glm::fvec2 velocity;
    float rotation;
    float rotationSpeed;
    float size;
    float alpha;
    glm::fvec3 color;
    std::unique_ptr<Mesh> mesh;

    PlayerFragment(glm::fvec2 pos, glm::fvec2 vel, float rot, float rotSpeed, float sz,
                   glm::fvec3 col);

    bool update(int deltaTime, GameState &) override;
    void draw(const GameState &) override;
private:
    void createMesh();
};
struct Player : Updatable, Drawable, Collidable {
    glm::fvec2 currentPosition;
    bool isBullet = false;
    int coolTime = 0;
    bool isInvincible = false;
    int invincibilityEndTime = 0;
    float tiltAngle = 0.0f;
    float targetTiltAngle = 0.0f;
    static constexpr int INVINCIBILITY_DURATION = 1200;
    bool isDying = false;
    int deathStartTime = 0;
    std::vector<PlayerFragment> fragments;
    int bulletCount = 3;
    std::vector<EnergyOrb> energyOrbs;

    // Static mesh for explosion effect (shared across all players)
    static std::unique_ptr<Mesh> explosionMesh;
    static void createExplosionMesh();

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
    std::function<float(float)> trajectory;
    std::function<float(float)> portion;

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

struct Boss : Updatable, Drawable, Collidable {
    glm::fvec2 currentPosition;
    BossMove currentMove;
    int bossId;
    int coolTime = 0;
    int coolTimePeriod = 500;
    bool isDying = false;
    int deathStartTime = 0;
    int lastHitTime = 0;
    float hitIntensity = 0.0f;

    Boss(glm::fvec2 initialPosition, int id = 1);

    void takeDamage(int currentTime);
    void startDeathAnimation(int currentTime);
    bool update(int currentTime, GameState &gameState) override;
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

struct Background : Drawable, Updatable {
    int lastUpdateTime;
    std::unique_ptr<Mesh> borderMesh;

    Background();
    bool update(int currentTime, GameState &gameState) override;
    void draw(const GameState &gameState) override;
private:
    void createBorderMesh();
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
    Skybox skyboxObject;

    std::vector<PlayerBullet> playerBulletObjects;
    std::vector<EnemyBullet> enemyBulletObjects;
    std::vector<TrailParticle> trailParticles;
    std::vector<std::shared_ptr<LightSource>> lights;
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

enum LightType {
    DIRECTIONAL_LIGHT = 0,
    POINT_LIGHT = 1
};

struct LightSource {
    LightType type;

    glm::fvec3 ambientColor;
    glm::fvec3 diffuseColor;
    glm::fvec3 specularColor;

    float intensity;
    bool enabled = true;

    LightSource(glm::fvec3 ambientColor, glm::fvec3 diffuseColor, glm::fvec3 specularColor,
                float intensity);

    virtual ~LightSource() = default;

    virtual void setUniforms(const ShaderProgram &program, int index) const = 0;
};

struct DirectionalLightSource : LightSource {
    glm::fvec3 direction;
    
    DirectionalLightSource(glm::fvec3 ambientColor, glm::fvec3 diffuseColor,
                           glm::fvec3 specularColor, float intensity, glm::fvec3 direction);

    void setUniforms(const ShaderProgram &program, int index) const override;
};

struct PointLightSource : LightSource {
    glm::fvec3 position;
    
    PointLightSource(glm::fvec3 ambientColor, glm::fvec3 diffuseColor, glm::fvec3 specularColor,
                     float intensity, glm::fvec3 position);

    glm::fvec3 getAttenuation(); // attenuation coefficient
    void setUniforms(const ShaderProgram &program, int index) const override;
};
