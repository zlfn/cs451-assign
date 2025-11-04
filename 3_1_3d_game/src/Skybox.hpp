#pragma once

#include <GL/glew.h>
#include <string>
#include <array>

struct GameState;

class Skybox {
public:
    Skybox();
    ~Skybox();

    bool load(const std::string& directory);
    void draw(const GameState& gameState);

private:
    GLuint textureID;
    bool loaded;

    bool loadCubemapFace(const std::string& path, GLenum target);
};
