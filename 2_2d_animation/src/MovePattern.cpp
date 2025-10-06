#include "base.hpp"

// 궤적 함수
auto traj1 = [](float u) { return u * (1.0f - u); };
auto por1 = [](float t) { return float(3 * t * t - 2 * t * t * t); };
auto zeroTraj = [](float u) { return 0.0f; };

// 움직임 함수
MoveFn boss1Move1 = [](int currentTime, GameState &gameState) {
    return BossMove(gameState.bossObject1.currentPosition, glm::fvec2(0.0f, 0.0f), 3000,
                    currentTime, traj1, por1);
};
MoveFn boss2Move1 = [](int currentTime, GameState &gameState) {
    return BossMove(gameState.bossObject2.currentPosition, glm::fvec2(0.0f, 0.6f), 3000,
                    currentTime, traj1, por1);
};
MoveFn boss1RandomMove = [](int currentTime, GameState &gameState) {
    float randomX = dist(gen) * 1.7f - 0.85f;
    float randomY = dist(gen) * 0.85f;

    return BossMove(gameState.bossObject1.currentPosition, glm::fvec2(randomX, randomY), 1000,
                    currentTime, zeroTraj, por1);
};
MoveFn boss2RandomMove = [](int currentTime, GameState &gameState) {
    float randomX = dist(gen) * 1.7f - 0.85f;
    float randomY = dist(gen) * 0.85f;

    return BossMove(gameState.bossObject2.currentPosition, glm::fvec2(randomX, randomY), 1000,
                    currentTime, zeroTraj, por1);
};

// 움직임 배열
static const std::vector<MoveEntry> BOSS_MOVE_LIST1 = {{
    {boss1Move1, 2000},
}};
static const std::vector<MoveEntry> BOSS_MOVE_LIST2 = {{
    {boss2Move1, 2000},
}};
static std::size_t boss1MoveListCounter = 0;
static std::size_t boss2MoveListCounter = 0;

// 움직임 함수 얻기
int random1Iteration = 0;
int random2Iteration = 0;
std::optional<BossMove> getCurrentMove(int currentTime, GameState &gameState, int bossNum) {
    std::size_t &bossMoveListCounter = (bossNum == 1) ? boss1MoveListCounter : boss2MoveListCounter;
    int &randomIteration = (bossNum == 1) ? random1Iteration : random2Iteration;
    const std::vector<MoveEntry> &currentBossMoveList =
        (bossNum == 1) ? BOSS_MOVE_LIST1 : BOSS_MOVE_LIST2;

    if (bossMoveListCounter >= currentBossMoveList.size()) {
        if (currentTime > 11000 + randomIteration * 5000) {
            randomIteration += 1;
            return (bossNum == 1) ? boss1RandomMove(currentTime, gameState)
                                  : boss2RandomMove(currentTime, gameState);
        }
        return std::nullopt;
    }

    const auto &[makeMove, startAt] = currentBossMoveList[bossMoveListCounter];

    if (currentTime >= startAt) {
        BossMove move = makeMove(currentTime, gameState);
        ++bossMoveListCounter;
        return move;
    }

    return std::nullopt;
}
