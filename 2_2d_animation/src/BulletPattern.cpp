#include "base.hpp"

float basePosFunc(int t, float speed) { return 0; }
float sqrtPosFunc1(int t, float speed) {
    float deltaX = static_cast<float>(t) * speed;
    return std::sqrt(deltaX);
}
float sqrtPosFunc2(int t, float speed) {
    float deltaX = static_cast<float>(t) * speed;
    return deltaX;
}

BulletVec bossEmptyPattern(GameState &gameState, int /*currentTime*/, int bossNum) {
    switch (bossNum) {
    case 1:
        gameState.bossObject1.coolTimePeriod = 500;
        break;
    case 2:
        gameState.bossObject2.coolTimePeriod = 500;
        break;
    default:
        break;
    }
    return {};
}
BulletVec bossBulletPattern1(GameState &gameState, int currentTime, int bossNum) {
    switch (bossNum) {
    case 1:
        gameState.bossObject1.coolTimePeriod = 500;
        break;
    case 2:
        gameState.bossObject2.coolTimePeriod = 500;
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