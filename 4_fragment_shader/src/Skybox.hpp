#pragma once

#include <GL/glew.h>
#include <string>
#include <array>
#include <memory>
#include "graphics.hpp"

struct GameState;

class Skybox {
  public:
    Skybox();
    ~Skybox();

    bool load(const std::string &directory);
    void draw(const GameState &gameState);

  private:
    GLuint textureID_;
    bool loaded_;
    std::unique_ptr<Mesh> cubeMesh_;
    std::unique_ptr<ShaderProgram> skyboxShader_;

    bool loadCubemapFace(const std::string &path, GLenum target);
    void createCubeMesh();
    void createShader();
};
