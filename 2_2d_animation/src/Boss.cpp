#include "base.hpp"

BossFragment::BossFragment(glm::fvec2 pos, glm::fvec2 vel, float rot, float rotSpeed, float sz,
                           glm::fvec3 col)
    : position(pos), velocity(vel), rotation(rot), rotationSpeed(rotSpeed), size(sz), alpha(1.0f),
      color(col) {}

bool BossFragment::update(int deltaTime, GameState &) {
    float dt = static_cast<float>(deltaTime) * 0.0005f; // Slower animation
    position += velocity * dt;
    rotation += rotationSpeed * dt;
    velocity.y -= 0.3f * dt;    // Slower gravity
    alpha -= dt * 0.15f;        // Slower fade out
    size *= (1.0f - dt * 0.1f); // Slower shrink
    return alpha <= 0.0f || size <= 0.001f;
}

void BossFragment::draw(const GameState &) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 기존 가산 블렌딩 유지

    glPushMatrix();
    glTranslatef(position.x, position.y, 0.0f); // 위치
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

BossArm::BossArm(bool isLeft, int boss)
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

void BossArm::update(float time) {
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

void BossArm::drawSegment(float length, float width, float brightness) {
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

void BossArm::drawJoint(float size) {
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

void BossArm::draw(const GameState &gameState) {
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

Boss::Boss(glm::fvec2 initialPosition, int id)
    : currentPosition(initialPosition), currentMove(idleBossMove(initialPosition, 0)),
      leftArm(true, id), rightArm(false, id) {}

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
void Boss::startDeathAnimation(int currentTime) {
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

void Boss::drawUnitCircleFan(int seg, float z, const glm::vec4 &centerRGBA,
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

// 반지름 1인 정팔각형 팬 (중심 포함)
void Boss::drawUnitOctagonFan(float z, const glm::vec4 &centerRGBA, const glm::vec4 &edgeRGBA) {
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

// 스파이크 1개 (원점에서 시작, 외내곽 반지름을 유닛으로 받음)
void Boss::drawUnitSpikeTri(float rOuter, float rInner, float z, const glm::vec4 &innerRGBA,
                            const glm::vec4 &tipRGBA) {
    glBegin(GL_TRIANGLES);
    glColor4f(innerRGBA.r, innerRGBA.g, innerRGBA.b, innerRGBA.a);
    glVertex3f(0.f, 0.f, z);
    glColor4f(tipRGBA.r, tipRGBA.g, tipRGBA.b, tipRGBA.a);
    glVertex3f(rOuter, 0.f, z);
    glVertex3f(rInner * std::cos(0.2f), rInner * std::sin(0.2f), z);
    glEnd();
}

// 메인 드로우
void Boss::draw(const GameState &gameState) {
    const float SIZE = 0.15f; // 전체 스케일(월드 단위)
    const float T = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.001f;

    if (isDying) {
        // 파편
        for (auto &fragment : fragments) {
            fragment.draw(gameState);
        }

        // 폭발
        const int NOW_MS = glutGet(GLUT_ELAPSED_TIME);
        const float DT = static_cast<float>(NOW_MS - deathStartTime) * 0.001f;
        if (DT < 1.5f) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 가산 혼합 유지

            const float EXPLOSION_SIZE = 0.3f * (1.0f + DT * 2.0f); // 느리게 팽창
            const float A = 1.0f - DT * 0.67f;                      // 느리게 페이드

            glPushMatrix();
            glTranslatef(currentPosition.x, currentPosition.y, 0.f);
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

    // 모델 행렬
    glPushMatrix();
    glTranslatef(currentPosition.x, currentPosition.y, 0.f);
    // glRotatef(angle, 0,0,1)
    glScalef(SIZE, SIZE, 1.f);

    // 본체
    drawUnitOctagonFan(0.0f, glm::vec4(0.3f, 0.0f, 0.8f, 1.0f), glm::vec4(0.1f, 0.0f, 0.6f, 1.0f));

    // 코어 반짝임
    glPushMatrix();
    glScalef(0.6f, 0.6f, 1.f);
    drawUnitCircleFan(20, 0.01f, glm::vec4(0.8f, 0.2f, 1.0f, 0.8f),
                      glm::vec4(0.4f, 0.0f, 0.8f, 0.2f));
    glPopMatrix();

    // 회전 스파이크
    glPushMatrix();
    glRotatef(T * 57.2957795f, 0.f, 0.f, 1.f); // rad -> deg
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

    // 코어 디테일
    glPushMatrix();
    glScalef(0.3f, 0.3f, 1.f);
    drawUnitCircleFan(10, 0.03f, glm::vec4(1.0f, 0.0f, 0.5f, 1.0f),
                      glm::vec4(0.3f, 0.0f, 0.2f, 1.0f));
    glPopMatrix();

    // 에너지 필드
    {
        const int SEG = 16;
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

    // 좌우 팔
    leftArm.draw(gameState);
    rightArm.draw(gameState);

    glPopMatrix(); // 모델 행렬 끝
    glDisable(GL_BLEND);
}
CollisionShape Boss::getShape() const {
    if (isDying) {
        // No collision when dying
        return CollisionCircle(glm::fvec2(-999.0f, -999.0f), 0.0f);
    }
    return CollisionCircle(currentPosition, 0.15f);
}
