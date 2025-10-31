#include "base.hpp"

ThreeDObj enemyBulletObj = ThreeDObj("assets/sphere.obj", glm::fvec3(1.0, 0.0, 0.0));
ThreeDObj enemyBulletSonicObj = ThreeDObj("assets/sonic.obj", glm::fvec3(0.2, 0.5, 0.5));
ThreeDObj playerBulletObj = ThreeDObj("assets/rice.obj", glm::fvec3(0.3, 0.4, 0.3));

EnemyBullet::EnemyBullet(glm::fvec2 initialDirection, glm::fvec2 initialPosition, float speed,
                         int initialTime, std::function<float(int, float)> posFunc)
    : initialDirection(glm::normalize(initialDirection) * speed),
      normalDirection(glm::normalize(glm::fvec2(-initialDirection.y, initialDirection.x))),
      initialPosition(initialPosition), currentPosition(initialPosition),
      previousPosition(initialPosition), initialTime(initialTime), lastTrailTime(initialTime),
      speed(speed), posFunc(std::move(posFunc)) {}

bool EnemyBullet::update(int currentTime, GameState &gameState) {
    previousPosition = currentPosition;
    int dt = currentTime - initialTime;
    currentPosition =
        initialPosition + float(dt) * initialDirection + posFunc(dt, speed) * normalDirection;

    // 트레일 파티클 생성
    if (currentTime - lastTrailTime > 15) {
        lastTrailTime = currentTime;

        for (int i = 0; i < 2; ++i) {
            std::uniform_real_distribution<float> offsetDist(-0.015f, 0.015f);
            std::uniform_real_distribution<float> velDist(-0.025f, 0.025f);

            glm::fvec2 trailPos = currentPosition + glm::fvec2(offsetDist(gen), offsetDist(gen));
            glm::fvec2 trailVel(velDist(gen), velDist(gen));
            float trailSize = 0.025f + offsetDist(gen) * 0.3f;

            glm::fvec3 trailColor(0.0f, 0.25f, 0.8f);

            gameState.trailParticles.emplace_back(trailPos, trailVel, trailSize, trailColor,
                                                  currentTime);
        }
    }

    // 보스가 죽어가는 중이 아닐 때만 플레이어에게 데미지
    if (!gameState.bossObject1.isDying && !gameState.playerObject.isInvincible &&
        detectCollision(*this, gameState.playerObject)) {
        gameState.playerHealth -= 1;
        gameState.playerObject.takeDamage(currentTime);
        startCameraShake(currentTime);
        if (gameState.playerHealth < 0)
            gameState.playerHealth = 0;
        return true;
    }
    return abs(currentPosition.x) > 2.0f || abs(currentPosition.y) > 2.0f;
}
void EnemyBullet::draw(const GameState &gameState) {
    const float BASE_SCALE = 0.03f;
    const float SONIC_RELATIVE_SCALE = 0.5f;

    const float T_MS = static_cast<float>(glutGet(GLUT_ELAPSED_TIME));
    glm::fvec2 deltaPosition = currentPosition - previousPosition;
    const float ROTATION_DEG =
        std::atan2(deltaPosition.y, deltaPosition.x) * 180.0f / std::numbers::pi_v<float>;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glPushMatrix();
    glTranslatef(currentPosition.x, currentPosition.y, 0.0f);
    glScalef(BASE_SCALE, BASE_SCALE, BASE_SCALE);

    enemyBulletObj.draw();

    glScalef(SONIC_RELATIVE_SCALE, SONIC_RELATIVE_SCALE, SONIC_RELATIVE_SCALE);
    glRotatef(ROTATION_DEG, 0.0, 0.0, 1.0);
    glTranslatef(5.0f, 0.0f, 0.0f);
    glRotatef(90, 0.0, 1.0, 0.0);
    enemyBulletSonicObj.draw();

    glPopMatrix();

    glDisable(GL_BLEND);
}

CollisionShape EnemyBullet::getShape() const { return CollisionCircle(currentPosition, 0.03f); }

PlayerBullet::PlayerBullet(glm::fvec2 initialPosition, float speed, int initialTime)
    : initialPosition(initialPosition), currentPosition(initialPosition), initialTime(initialTime),
      speed(speed) {}

bool PlayerBullet::update(int currentTime, GameState &gameState) {
    currentPosition =
        initialPosition + glm::fvec2(0, speed * static_cast<float>(currentTime - initialTime));

    // 플레이어가 죽어가는 중이 아닐 때만 보스에게 데미지
    bool hitBoss1 = detectCollision(*this, gameState.bossObject1);
    bool hitBoss2 = detectCollision(*this, gameState.bossObject2);

    if (!gameState.playerObject.isDying && (hitBoss1 || hitBoss2)) {
        gameState.bossHealth -= 1;
        if (gameState.bossHealth < 0)
            gameState.bossHealth = 0;

        // 피격된 보스에게 피격 효과 적용
        if (hitBoss1)
            gameState.bossObject1.takeDamage(currentTime);
        if (hitBoss2)
            gameState.bossObject2.takeDamage(currentTime);

        return true;
    }
    return abs(currentPosition.x) > 2.0f || abs(currentPosition.y) > 2.0f;
}
void PlayerBullet::draw(const GameState &gameState) {
    const float SCALE = 0.01f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glPushMatrix();
    glTranslatef(currentPosition.x, currentPosition.y, 0.0f);
    glScalef(SCALE, SCALE, SCALE);
    glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    
    playerBulletObj.draw();

    glPopMatrix();

    glDisable(GL_BLEND);
}
CollisionShape PlayerBullet::getShape() const {
    return CollisionRectangle(currentPosition - glm::fvec2(0.015f, 0.015f),
                              currentPosition + glm::fvec2(0.015f, 0.015f));
}
