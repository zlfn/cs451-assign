#include "base.hpp"
#include "utils.hpp"

PlayerFragment::PlayerFragment(glm::fvec2 pos, glm::fvec2 vel, float rot, float rotSpeed, float sz,
                               glm::fvec3 col)
    : position(pos), velocity(vel), rotation(rot), rotationSpeed(rotSpeed), size(sz), alpha(1.0f),
      color(col) {}

bool PlayerFragment::update(int deltaTime, GameState &) {
    float dt = static_cast<float>(deltaTime) * 0.0005f;
    position += velocity * dt;
    rotation += rotationSpeed * dt;
    velocity.y -= 0.4f * dt;
    alpha -= dt * 0.2f;
    size *= (1.0f - dt * 0.15f);
    return alpha <= 0.0f || size <= 0.001f;
}

void PlayerFragment::draw(const GameState &) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glPushMatrix();
    glTranslatef(position.x, position.y, 0.0f);
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

EnergyOrb::EnergyOrb()
    : offset(0.0f, 0.0f), angle(0.0f), orbitRadius(0.08f), size(0.025f), birthTime(0),
      playerPosition(0.0f, 0.0f) {
    updatePosition();
}

EnergyOrb::EnergyOrb(float startAngle, float radius, float sz, int currentTime)
    : offset(0.0f, 0.0f), angle(startAngle), orbitRadius(radius), size(sz), birthTime(currentTime),
      playerPosition(0.0f, 0.0f) {
    updatePosition();
}

void EnergyOrb::updatePosition() {
    offset.x = orbitRadius * std::cos(angle);
    offset.y = orbitRadius * std::sin(angle);
}

void EnergyOrb::setPlayerPosition(const glm::fvec2 &pos) { playerPosition = pos; }

bool EnergyOrb::update(int currentTime, GameState &) {
    // 각도는 Player::updateEnergyOrbs에서 관리되므로 여기서는 위치만 업데이트
    updatePosition();
    return false; // 에너지 구체는 자동으로 사라지지 않음
}

void EnergyOrb::draw(const GameState &) {
    glm::fvec2 worldPos = playerPosition + offset;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glPushMatrix();
    glTranslatef(worldPos.x, worldPos.y, 0.1f);
    glScalef(size, size, 1.0f);

    // 외부 글로우 효과
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 0.5f, 0.0f, 0.3f); // 중심: 주황색, 투명도 낮음
    glVertex3f(0.0f, 0.0f, 0.0f);

    glColor4f(1.0f, 0.3f, 0.0f, 0.0f); // 가장자리: 투명
    const int SEGMENTS = 12;
    for (int i = 0; i <= SEGMENTS; ++i) {
        float segAngle =
            static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(SEGMENTS);
        glVertex3f(1.5f * std::cos(segAngle), 1.5f * std::sin(segAngle), 0.0f);
    }
    glEnd();

    // 내부 핵심부
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 0.8f, 0.2f, 0.9f); // 중심: 밝은 주황색
    glVertex3f(0.0f, 0.0f, 0.01f);

    glColor4f(1.0f, 0.5f, 0.0f, 0.7f); // 가장자리: 어두운 주황색
    for (int i = 0; i <= SEGMENTS; ++i) {
        float segAngle =
            static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(SEGMENTS);
        glVertex3f(0.8f * std::cos(segAngle), 0.8f * std::sin(segAngle), 0.01f);
    }
    glEnd();

    glPopMatrix();
    glDisable(GL_BLEND);
}

Player::Player(glm::fvec2 initialPosition) : currentPosition(initialPosition) {}

void Player::updateEnergyOrbs(int currentHealth, int currentTime) {
    int currentOrbCount = static_cast<int>(energyOrbs.size());

    if (currentHealth > currentOrbCount) {
        // 체력이 증가했을 때 구체 추가
        for (int i = currentOrbCount; i < currentHealth; ++i) {
            float startAngle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> /
                               static_cast<float>(currentHealth);
            energyOrbs.emplace_back(startAngle, 0.08f, 0.025f, currentTime);
        }
    } else if (currentHealth < currentOrbCount) {
        // 체력이 감소했을 때 구체 제거 (뒤에서부터)
        energyOrbs.resize(currentHealth);
    }

    // 구체들을 현재 체력 개수에 맞게 균등하게 배치
    for (int i = 0; i < static_cast<int>(energyOrbs.size()); ++i) {
        // 각 구체의 고정된 시작 각도 (균등 분배)
        float baseAngle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> /
                          static_cast<float>(energyOrbs.size());

        // 전체적으로 회전하는 각도 (시간에 따라 변화)
        float rotationSpeed = std::numbers::pi_v<float> / 2.0f; // 초당 90도
        float globalRotation = rotationSpeed * static_cast<float>(currentTime) * 0.001f;

        // 최종 각도 = 기본 위치 + 전체 회전
        energyOrbs[i].angle = baseAngle + globalRotation;

        // 각도 정규화
        while (energyOrbs[i].angle > 2.0f * std::numbers::pi_v<float>) {
            energyOrbs[i].angle -= 2.0f * std::numbers::pi_v<float>;
        }
        while (energyOrbs[i].angle < 0.0f) {
            energyOrbs[i].angle += 2.0f * std::numbers::pi_v<float>;
        }
    }
}

void Player::startDeathAnimation(int currentTime) {
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

    // 에너지 구체들 업데이트
    for (auto &orb : energyOrbs) {
        orb.setPlayerPosition(currentPosition);
        orb.update(currentTime, gameState);
    }

    glm::fvec2 &cbo = gameState.cameraBaseOffset;
    if (cbo.x - currentPosition.x >= 0.6) {
        cbo.x = currentPosition.x + 0.6f;
    } else if (cbo.x - currentPosition.x <= -0.6) {
        cbo.x = currentPosition.x - 0.6f;
    }
    if (cbo.y - currentPosition.y >= 0.6) {
        cbo.y = currentPosition.y + 0.6f;
    } else if (cbo.y - currentPosition.y <= 0.2) {
        cbo.y = currentPosition.y + 0.2f;
    }

    return false;
}

void Player::tryAttack() {
    if (!isDying)
        isBullet = true;
}
void Player::draw(const GameState &gameState) {
    if (isDying) {
        // 파편은 그대로(파편 내부에서 동일한 방식으로 모델행렬 쓰는 게 이상적)
        for (auto &fragment : fragments) {
            fragment.draw(gameState);
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
            glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(currentPosition, 0.0f));
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
    glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(currentPosition, 0.0f));
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

    // 에너지 구체들 그리기 (플레이어 위에 그려지도록)
    for (auto &orb : energyOrbs) {
        orb.setPlayerPosition(currentPosition);
        orb.draw(gameState);
    }
}
void Player::move(glm::fvec2 deltaPosition) {
    if (isDying)
        return; // No movement when dying

    currentPosition += deltaPosition;

    // Clamp
    if (currentPosition.x < -2.0f)
        currentPosition.x = -2.0f;
    if (currentPosition.x > 2.0f)
        currentPosition.x = 2.0f;
    if (currentPosition.y < -2.0f)
        currentPosition.y = -2.0f;
    if (currentPosition.y > 2.0f)
        currentPosition.y = 2.0f;
}
void Player::takeDamage(int currentTime) {
    if (!isInvincible) {
        isInvincible = true;
        invincibilityEndTime = currentTime + INVINCIBILITY_DURATION;
    }
}
CollisionShape Player::getShape() const {
    if (isDying) {
        // No collision when dying
        return CollisionCircle(glm::fvec2(-999.0f, -999.0f), 0.0f);
    }
    return CollisionRectangle(currentPosition - glm::fvec2(0.015f, 0.015f + 0.025f),
                              currentPosition + glm::fvec2(0.015f, 0.015f - 0.025f));
}
