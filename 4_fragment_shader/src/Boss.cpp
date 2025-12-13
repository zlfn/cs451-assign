#include "base.hpp"

ThreeDObj& getBossObj() {
    static ThreeDObj obj("assets/drone.obj", "assets/diffuse_starship.png",
                              "assets/normal_industrial.png", glm::fvec3(0.0, 1.0, 0.0));
    return obj;
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
    getBossObj().splitObjectByZPlane("Body");
}

void bossDrawPropeller(const std::string objName, const float DT, const float speed) {
    const float SCALE = 1.5;
    glm::vec3 ct = getBossObj().objCenterMap[objName];
    modelViewStack.matPush();
    modelViewStack.translate(ct.x, ct.y, ct.z);
    modelViewStack.rotate(DT * 360.0 * speed, 0.0, 1.0, 0.0);
    modelViewStack.scale(SCALE, SCALE, SCALE);
    modelViewStack.translate(-ct.x, -ct.y, -ct.z);
    getBossObj().draw(objName);
    modelViewStack.matPop();
}

// 보스 그리기
void Boss::draw(const GameState &gameState) {
    const float SIZE = 0.4f;
    const int NOW_MS = glutGet(GLUT_ELAPSED_TIME);
    const float DT = static_cast<float>(NOW_MS - deathStartTime) * 0.001f;

    modelViewStack.matPush();
    modelViewStack.translate(currentPosition.x, currentPosition.y, 0.f);
    modelViewStack.scale(SIZE, SIZE, SIZE);
    modelViewStack.rotate(90.0, 1.0, 0.0, 0.0);
    if (isDying) {
        getBossObj().separate(0.001);
        getBossObj().drawAll();

    }
    else {
        getBossObj().draw("Body");
        getBossObj().draw("Cube.002");

        bossDrawPropeller("Rotor_FL", DT, 3.0);
        bossDrawPropeller("Rotor_FR", DT, -3.0);
        bossDrawPropeller("Rotor_BL", DT, 3.0);
        bossDrawPropeller("Rotor_BR", DT, -3.0);
    }

    modelViewStack.matPop();
}

CollisionShape Boss::getShape() const {
    if (isDying) {
        return CollisionCircle(glm::fvec2(-999.0f, -999.0f), 0.0f);
    }
    return CollisionCircle(currentPosition, 0.1f);
}
