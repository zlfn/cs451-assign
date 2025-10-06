#include "base.hpp"

CommandExecutor::CommandExecutor(std::vector<char> sequence,
                                 std::function<void(GameState &)> activateFunc)
    : commandSequence(std::move(sequence)), onActivate(std::move(activateFunc)) {}

bool CommandExecutor::update(int currentTime, GameState &gameState) {
    // Check for key press events (rising edge detection)
    for (int i = 0; i < 256; i++) {
        if (keyStates[i] && !previousKeyStates[i]) {
            // Key was just pressed
            handleKeyPress(static_cast<char>(i), gameState);
        }
        previousKeyStates[i] = keyStates[i];
    }
    return false;
}

void CommandExecutor::handleKeyPress(char key, GameState &gameState) {
    if (commandUsed)
        return;

    // Check if the pressed key matches the current position in the sequence
    if (currentIndex < commandSequence.size() && key == commandSequence[currentIndex]) {
        currentIndex++;

        // Check if the entire sequence has been completed
        if (currentIndex >= commandSequence.size()) {
            activateCommand(gameState);
        }
    } else {
        // Reset if wrong key pressed, but check if this key could start the sequence
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
