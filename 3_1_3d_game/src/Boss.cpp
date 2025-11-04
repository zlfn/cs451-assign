#include "base.hpp"

ThreeDObj bossObj = ThreeDObj("assets/drone.obj", glm::fvec3(0.0, 1.0, 0.0));

BossFragment::BossFragment(glm::fvec2 pos, glm::fvec2 vel, float rot, float rotSpeed, float sz,
                           glm::fvec3 col)
    : position(pos), velocity(vel), rotation(rot), rotationSpeed(rotSpeed), size(sz), alpha(1.0f),
      color(col) {}

bool BossFragment::update(int deltaTime, GameState &) {
    float dt = static_cast<float>(deltaTime) * 0.0005f;
    position += velocity * dt;
    rotation += rotationSpeed * dt;
    velocity.y -= 0.3f * dt;    // 중력 효과
    alpha -= dt * 0.15f;        // 페이드 아웃
    size *= (1.0f - dt * 0.1f); // 크기 축소
    return alpha <= 0.0f || size <= 0.001f;
}

void BossFragment::draw(const GameState &) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glPushMatrix();
    glTranslatef(position.x, position.y, 0.0f);
    glRotatef(rotation, 0.0f, 0.0f, 1.0f);
    glScalef(size, size, 1.0f);

    // 단위 정삼각형
    constexpr float H = 0.8660254f; // sqrt(3)/2

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

Boss::Boss(glm::fvec2 initialPosition, int id)
    : currentPosition(initialPosition), currentMove(idleBossMove(initialPosition, 0)), bossId(id) {}

void Boss::takeDamage(int currentTime) {
    lastHitTime = currentTime;
    hitIntensity = 1.0f;
}

bool Boss::update(int currentTime, GameState &gameState) {
    // 피격 강도 감쇠
    if (hitIntensity > 0.0f) {
        float timeSinceHit = static_cast<float>(currentTime - lastHitTime) * 0.001f;
        hitIntensity = std::max(0.0f, 1.0f - timeSinceHit * 3.0f);
    }

    if (isDying) {
        // 파편 업데이트
        int deltaTime = currentTime - deathStartTime;
        std::erase_if(fragments, [deltaTime, &gameState](auto &fragment) {
            return fragment.update(deltaTime, gameState);
        });

        // 3초 후 승리 화면 표시
        if ((currentTime - deathStartTime) > 3000) {
            showVictoryScreen(gameState);
            std::exit(0);
        }

        return (currentTime - deathStartTime) > 5000;
    }

    // 보스가 죽어야 하는지 확인
    if (gameState.bossHealth <= 0 && !isDying) {
        startDeathAnimation(currentTime);
        startCameraShake(currentTime);
        return false;
    }

    this->currentPosition = this->currentMove.getCurrentPosition(currentTime);

    // 보스가 죽어가는 중이 아닐 때만 플레이어에게 데미지
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

    auto newBullets = getCurrentBulletPattern(currentTime)(gameState, currentTime, bossId);
    gameState.enemyBulletObjects.insert(gameState.enemyBulletObjects.end(), newBullets.begin(),
                                        newBullets.end());

    return false;
}
// 보스 사망 애니메이션 시작
void Boss::startDeathAnimation(int currentTime) {
    if (isDying)
        return;
    isDying = true;
    deathStartTime = currentTime;

    // 파편 생성
    std::uniform_real_distribution<float> speedDist(0.3f, 1.2f);
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * std::numbers::pi_v<float>);
    std::uniform_real_distribution<float> rotSpeedDist(-180.0f, 180.0f);
    std::uniform_real_distribution<float> sizeDist(0.03f, 0.1f);

    for (int i = 0; i < 20; ++i) {
        float speed = speedDist(gen);
        float angle = angleDist(gen);
        glm::fvec2 vel(speed * std::cos(angle), speed * std::sin(angle));
        float rotSpeed = rotSpeedDist(gen);
        float size = sizeDist(gen);

        glm::fvec3 color;
        if (i % 3 == 0) {
            color = glm::fvec3(1.0f, 0.2f, 0.8f);
        } else if (i % 3 == 1) {
            color = glm::fvec3(0.6f, 0.1f, 1.0f);
        } else {
            color = glm::fvec3(0.8f, 0.4f, 1.0f);
        }

        fragments.emplace_back(currentPosition, vel, 0.0f, rotSpeed, size, color);
    }
}

void bossDrawPropeller(const std::string objName, const float DT, const float speed) {
    const float SCALE = 1.5;
    glm::vec3 ct = bossObj.objCenterMap[objName];
    glPushMatrix();
    glTranslatef(ct.x, ct.y, ct.z);
    glRotatef(DT * 360.0 * speed, 0.0, 1.0, 0.0);
    glScalef(SCALE, SCALE, SCALE);
    glTranslatef(-ct.x, -ct.y, -ct.z);
    bossObj.draw(objName);
    glPopMatrix();
}

// 보스 그리기
void Boss::draw(const GameState &gameState) {
    const float SIZE = 0.4f;
    const int NOW_MS = glutGet(GLUT_ELAPSED_TIME);
    const float DT = static_cast<float>(NOW_MS - deathStartTime) * 0.001f;

    if (isDying) {
        for (auto &fragment : fragments) {
            fragment.draw(gameState);
        }

        // 폭발 효과
        if (DT < 1.5f) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            const float EXPLOSION_SIZE = 0.3f * (1.0f + DT * 2.0f);

            glPushMatrix();
            glTranslatef(currentPosition.x, currentPosition.y, 0.f);
            glScalef(EXPLOSION_SIZE, EXPLOSION_SIZE, 1.f);
            glPopMatrix();

            glDisable(GL_BLEND);
        }
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();
    glTranslatef(currentPosition.x, currentPosition.y, 0.f);
  
    glScalef(SIZE, SIZE, SIZE);

    glRotatef(90.0, 1.0, 0.0, 0.0);
    bossObj.draw("Body");
    bossObj.draw("Cube.002");

    bossDrawPropeller("Rotor_FL", DT, 3.0);
    bossDrawPropeller("Rotor_FR", DT, -3.0);
    bossDrawPropeller("Rotor_BL", DT, 3.0);
    bossDrawPropeller("Rotor_BR", DT, -3.0);

    glPopMatrix();
    glDisable(GL_BLEND);
}

CollisionShape Boss::getShape() const {
    if (isDying) {
        return CollisionCircle(glm::fvec2(-999.0f, -999.0f), 0.0f);
    }
    return CollisionCircle(currentPosition, 0.1f);
}
