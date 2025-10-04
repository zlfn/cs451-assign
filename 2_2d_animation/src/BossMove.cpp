#include "base.hpp"

glm::fvec2 BossMove::getCurrentPosition(int currentTime) {
    if (currentTime <= initialTime)
        return origin;
    if (currentTime >= initialTime + travelTime)
        return destination;
    float timePortion =
        portion(static_cast<float>(currentTime - initialTime) / ((float)travelTime));
    glm::fvec2 currentPosition =
        origin + directionVector * timePortion + trajectory(timePortion) * normalVector;
    return currentPosition;
}

BossMove idleBossMove(glm::fvec2 position, int startTime) {
    auto trivialFunc = [](float) { return 0.0f; };
    auto trivialFuncPor = [](float t) { return t; };
    return BossMove(position, position, 0, startTime, trivialFunc, trivialFuncPor);
}
