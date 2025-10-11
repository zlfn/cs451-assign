#include "base.hpp"

CommandExecutor::CommandExecutor(std::vector<char> sequence,
                                 std::function<void(GameState &)> activateFunc)
    : commandSequence(std::move(sequence)), onActivate(std::move(activateFunc)) {}

bool CommandExecutor::update(int currentTime, GameState &gameState) {
    // 키 입력 감지 (상승 엣지)
    for (int i = 0; i < 256; i++) {
        if (keyStates[i] && !previousKeyStates[i]) {
            handleKeyPress(static_cast<char>(i), gameState);
        }
        previousKeyStates[i] = keyStates[i];
    }
    return false;
}

void CommandExecutor::handleKeyPress(char key, GameState &gameState) {
    if (commandUsed)
        return;

    // 입력한 키가 시퀀스의 현재 위치와 일치하는지 확인
    if (currentIndex < commandSequence.size() && key == commandSequence[currentIndex]) {
        currentIndex++;

        // 전체 시퀀스 완료 확인
        if (currentIndex >= commandSequence.size()) {
            activateCommand(gameState);
        }
    } else {
        // 잘못된 키를 눌렀지만 시퀀스의 첫 번째 키인 경우
        if (!commandSequence.empty() && key == commandSequence[0]) {
            currentIndex = 1;
        } else {
            currentIndex = 0;
        }
    }
}

void CommandExecutor::activateCommand(GameState &gameState) {
    if (commandUsed)
        return;

    commandUsed = true;
    onActivate(gameState);
}
