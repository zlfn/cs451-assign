#include "base.hpp"
#include "utils.hpp"
#include "graphics.hpp"

ThreeDObj& getJetObj() {
    static ThreeDObj obj("assets/jet.obj", "assets/diffuse_jet.png", "assets/normal_flat.png",
                             glm::fvec3(1.0, 1.0, 0.0));
    return obj;
}

ThreeDObj& getEnergyOrbObj() {
    static ThreeDObj obj("assets/star.obj", "assets/diffuse_star.png",
                                   "assets/normal_quilt.png", glm::fvec3(1.0, 1.0, 1.0));
    return obj;
}

// Static mesh for explosion effect
std::unique_ptr<Mesh> Player::explosionMesh = nullptr;

// Create explosion mesh once and reuse it
void Player::createExplosionMesh() {
    if (explosionMesh) return;  // Already created

    const int N = 20;
    std::vector<float> vertices;
    vertices.reserve((N * 3) * 6);

    glm::vec3 centerColor(1.0f, 0.9f, 0.0f);
    glm::vec3 edgeColor(1.0f, 0.5f, 0.0f);

    // Create a circle as triangle fan (converted to triangles)
    for (int i = 0; i < N; ++i) {
        float angle1 = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(N);
        float angle2 = static_cast<float>(i + 1) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(N);

        // Center point
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(centerColor.r); vertices.push_back(centerColor.g); vertices.push_back(centerColor.b);

        // First edge point
        vertices.push_back(std::cos(angle1)); vertices.push_back(std::sin(angle1)); vertices.push_back(0.0f);
        vertices.push_back(edgeColor.r); vertices.push_back(edgeColor.g); vertices.push_back(edgeColor.b);

        // Second edge point
        vertices.push_back(std::cos(angle2)); vertices.push_back(std::sin(angle2)); vertices.push_back(0.0f);
        vertices.push_back(edgeColor.r); vertices.push_back(edgeColor.g); vertices.push_back(edgeColor.b);
    }

    explosionMesh = std::make_unique<Mesh>();
    explosionMesh->setData(vertices.data(), vertices.size() * sizeof(float), GL_STATIC_DRAW);
    explosionMesh->setAttribute(0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
    explosionMesh->setAttribute(1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    explosionMesh->setDrawMode(GL_TRIANGLES, N * 3);
}

EnergyOrb::EnergyOrb(float startAngle, float radius, float sz, int currentTime)
    : offset(0.0f, 0.0f), angle(startAngle), orbitRadius(radius), size(sz), birthTime(currentTime) {
    updatePosition();
}

void EnergyOrb::updatePosition() {
    offset.x = orbitRadius * std::cos(angle);
    offset.y = orbitRadius * std::sin(angle);
}

bool EnergyOrb::update(int currentTime, GameState &) { return false; }

void EnergyOrb::draw(const GameState &) {
    const glm::fvec2 VIEW_POS = offset;

    modelViewStack.matPush();
    modelViewStack.translate(VIEW_POS.x, VIEW_POS.y, 0.1f);
    modelViewStack.scale(size, size, size);
    modelViewStack.rotate(90.0, 1.0, 0.0, 0.0);

    getEnergyOrbObj().draw("Sphere");

    modelViewStack.matPop();
}

PlayerFragment::PlayerFragment(glm::fvec2 pos, glm::fvec2 vel, float rot, float rotSpeed, float sz,
                               glm::fvec3 col)
    : position(pos), velocity(vel), rotation(rot), rotationSpeed(rotSpeed), size(sz), alpha(1.0f),
      color(col) {
    createMesh();
}

bool PlayerFragment::update(int deltaTime, GameState &) {
    float dt = static_cast<float>(deltaTime) * 0.0005f;
    position += velocity * dt;
    rotation += rotationSpeed * dt;
    velocity.y -= 0.4f * dt;
    alpha -= dt * 0.2f;
    size *= (1.0f - dt * 0.15f);
    return alpha <= 0.0f || size <= 0.001f;
}

void PlayerFragment::createMesh() {
    const float H = std::sqrt(3.0f) / 2.0f;

    float vertices[] = {
        0.0f, 1.0f, 0.0f,  color.r, color.g, color.b,
        -H, -0.5f, 0.0f,   color.r * 0.5f, color.g * 0.5f, color.b * 0.5f,
        H, -0.5f, 0.0f,    color.r * 0.5f, color.g * 0.5f, color.b * 0.5f
    };

    mesh = std::make_unique<Mesh>();
    mesh->setData(vertices, sizeof(vertices), GL_STATIC_DRAW);
    mesh->setAttribute(0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
    mesh->setAttribute(1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    mesh->setDrawMode(GL_TRIANGLES, 3);
}

void PlayerFragment::draw(const GameState &) {
    if (!g_shaderProgram || !mesh) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    modelViewStack.matPush();
    modelViewStack.translate(position.x, position.y, 0.0f);
    modelViewStack.rotate(rotation, 0.0f, 0.0f, 1.0f);
    modelViewStack.scale(size, size, 1.0f);

    glm::mat4 projection = projectionStack.getTopMatrix();
    glm::mat4 modelView = modelViewStack.getTopMatrix();
    glm::mat4 normalMat = modelViewStack.getTopNormal();

    drawMesh(*mesh, *g_shaderProgram, [&](const ShaderProgram& prog) {
        prog.setUniform("projection", projection);
        prog.setUniform("modelView", modelView);
        prog.setUniform("normalMatrix", normalMat);
        prog.setUniform("objectColor", color);
        prog.setUniform("useVertexColor", 1.0f);
    });

    modelViewStack.matPop();
    glDisable(GL_BLEND);
}

Player::Player(glm::fvec2 initialPosition) : currentPosition(initialPosition) {}

// 플레이어 사망 애니메이션 시작
void Player::startDeathAnimation(int currentTime) {
    if (isDying)
        return;
    isDying = true;
    deathStartTime = currentTime;

    // 파편 생성
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
            color = glm::fvec3(1.0f, 1.0f, 0.0f);
        } else if (i % 3 == 1) {
            color = glm::fvec3(1.0f, 0.6f, 0.0f);
        } else {
            color = glm::fvec3(1.0f, 0.8f, 0.2f);
        }

        fragments.emplace_back(currentPosition, vel, 0.0f, rotSpeed, size, color);
    }
}

// 에너지 구체 업데이트
void Player::updateEnergyOrbs(int currentHealth, int currentTime) {
    int currentOrbCount = static_cast<int>(energyOrbs.size());

    if (currentHealth > currentOrbCount) {
        for (int i = currentOrbCount; i < currentHealth; ++i) {
            float startAngle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> /
                               static_cast<float>(currentHealth);
            energyOrbs.emplace_back(startAngle, 0.6f, 0.2f, currentTime);
        }
    } else if (currentHealth < currentOrbCount) {
        energyOrbs.erase(energyOrbs.begin() + currentHealth, energyOrbs.end());
    }

    // 구체 회전 및 배치
    for (int i = 0; i < static_cast<int>(energyOrbs.size()); ++i) {
        float baseAngle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> /
                          static_cast<float>(energyOrbs.size());
        float rotationSpeed = std::numbers::pi_v<float> / 2.0f;
        float globalRotation = rotationSpeed * static_cast<float>(currentTime) * 0.001f;

        energyOrbs[i].angle = baseAngle + globalRotation;

        // 각도 정규화
        while (energyOrbs[i].angle > 2.0f * std::numbers::pi_v<float>) {
            energyOrbs[i].angle -= 2.0f * std::numbers::pi_v<float>;
        }
        while (energyOrbs[i].angle < 0.0f) {
            energyOrbs[i].angle += 2.0f * std::numbers::pi_v<float>;
        }

        energyOrbs[i].updatePosition();
    }
}

bool Player::update(int currentTime, GameState &gameState) {
    if (isDying) {
        // 파편 업데이트
        int deltaTime = currentTime - deathStartTime;
        std::erase_if(fragments, [deltaTime, &gameState](auto &fragment) {
            return fragment.update(deltaTime, gameState);
        });

        // 4초 후 게임 종료
        if ((currentTime - deathStartTime) > 4000) {
            std::cout << "Game Over! Exiting...\n";
            std::exit(0);
        }

        return false;
    }

    // 플레이어가 죽어야 하는지 확인
    if (gameState.playerHealth <= 0 && !isDying) {
        startDeathAnimation(currentTime);
        startCameraShake(currentTime);
        return false;
    }

    // 무적 시간 종료 확인
    if (isInvincible && currentTime >= invincibilityEndTime) {
        isInvincible = false;
    }

    float tiltSpeed = 0.35f;
    if (std::abs(targetTiltAngle - tiltAngle) > 0.1f) {
        tiltAngle += (targetTiltAngle - tiltAngle) * tiltSpeed;
    } else {
        tiltAngle = targetTiltAngle;
    }

    // 총알 발사
    if (currentTime >= this->coolTime && this->isBullet) {
        float spacing = 0.03f;
        float totalWidth = spacing * static_cast<float>(bulletCount - 1);
        float startX = -totalWidth / 2.0f;
        float yForwardOffset = 0.04f; // 중앙 총알이 더 앞으로

        for (int i = 0; i < bulletCount; ++i) {
            float xOffset = startX + (spacing * static_cast<float>(i));
            // 중앙 총알일수록 더 앞으로 배치
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

    updateEnergyOrbs(gameState.playerHealth, currentTime);

    glm::fvec2 &cbo = gameState.cameraBaseOffset;
    cbo.x = currentPosition.x;
    cbo.y = currentPosition.y;

    return false;
}

void Player::tryAttack() {
    if (!isDying)
        isBullet = true;
}
void Player::draw(const GameState &gameState) {
    const float SCALE = 0.01;
    const float ORB_SCALE = 0.2;
    if (isDying) {
        for (auto &fragment : fragments) {
            fragment.draw(gameState);
        }

        // 폭발 효과
        int currentTime = glutGet(GLUT_ELAPSED_TIME);
        float timeSinceDeath = static_cast<float>(currentTime - deathStartTime) * 0.001f;

        if (timeSinceDeath < 1.2f && g_shaderProgram) {
            // Create explosion mesh once if not already created
            if (!explosionMesh) {
                createExplosionMesh();
            }

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            float explosionSize = 0.2f * (1.0f + timeSinceDeath * 3.0f);
            float alpha = 1.0f - timeSinceDeath * 0.83f;

            modelViewStack.matPush();
            modelViewStack.translate(currentPosition.x, currentPosition.y, 0.0f);
            modelViewStack.scale(explosionSize, explosionSize, 1.0f);

            glm::vec3 centerColor(1.0f, 0.9f, 0.0f);

            glm::mat4 projection = projectionStack.getTopMatrix();
            glm::mat4 modelView = modelViewStack.getTopMatrix();

            // Reuse static mesh instead of creating new one every frame
            drawMesh(*explosionMesh, *g_shaderProgram, [&](const ShaderProgram& prog) {
                prog.setUniform("projection", projection);
                prog.setUniform("modelView", modelView);
                prog.setUniform("objectColor", centerColor);
                prog.setUniform("useVertexColor", 1.0f);
            });

            modelViewStack.matPop();
            glDisable(GL_BLEND);
        }
        return;
    }

    modelViewStack.matPush();
    modelViewStack.translate(currentPosition.x, currentPosition.y, 0.0f);
    modelViewStack.matPush();
    modelViewStack.rotate(-90.0f, 1.0f, 0.0f, 0.0f);
    modelViewStack.scale(SCALE, SCALE, SCALE);

    if (isInvincible) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        getJetObj().draw("base");
        glDisable(GL_BLEND);
    } else {
        getJetObj().draw("base");
    }

    modelViewStack.matPop();
    modelViewStack.scale(ORB_SCALE, ORB_SCALE, ORB_SCALE);
    for (auto &orb : energyOrbs) {
        orb.draw(gameState);
    }

    modelViewStack.matPop();
}
void Player::move(glm::fvec2 deltaPosition) {
    if (isDying)
        return;

    currentPosition += deltaPosition;

    // 위치 제한
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
        return CollisionCircle(glm::fvec2(-999.0f, -999.0f), 0.0f);
    }
    return CollisionRectangle(currentPosition - glm::fvec2(0.015f, 0.015f + 0.025f),
                              currentPosition + glm::fvec2(0.015f, 0.015f - 0.025f));
}
