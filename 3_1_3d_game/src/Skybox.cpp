#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Skybox.hpp"
#include "base.hpp"
#include <iostream>

Skybox::Skybox() : textureID_(0), loaded_(false) {}

Skybox::~Skybox() {
    if (textureID_ != 0) {
        glDeleteTextures(1, &textureID_);
    }
}

bool Skybox::loadCubemapFace(const std::string &path, GLenum target) {
    int width = 0, height = 0, channels = 0;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 4);

    if (!data) {
        std::cerr << "Failed to load texture: " << path << '\n';
        std::cerr << "STB Error: " << stbi_failure_reason() << '\n';
        return false;
    }

    glTexImage2D(target, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    return true;
}

bool Skybox::load(const std::string &directory) {
    if (textureID_ == 0) {
        glGenTextures(1, &textureID_);
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID_);

    std::array<std::string, 6> faces = {
        directory + "/right.png",  // GL_TEXTURE_CUBE_MAP_POSITIVE_X
        directory + "/left.png",   // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
        directory + "/top.png",    // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
        directory + "/bottom.png", // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
        directory + "/front.png",  // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
        directory + "/back.png"    // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    for (size_t i = 0; i < faces.size(); i++) {
        if (!loadCubemapFace(faces[i], GL_TEXTURE_CUBE_MAP_POSITIVE_X + i)) {
            loaded_ = false;
            return false;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Enable seamless cubemap to remove seams between faces
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    loaded_ = true;
    return true;
}

void Skybox::draw(const GameState & /*gameState*/) {
    if (!loaded_)
        return;

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glEnable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID_);

    const float SIZE = 14.0f;

    glColor3f(1.0f, 1.0f, 1.0f);

    // Front face (Positive Z)
    glBegin(GL_QUADS);
    glTexCoord3f(SIZE, -SIZE, SIZE);
    glVertex3f(SIZE, -SIZE, SIZE);
    glTexCoord3f(-SIZE, -SIZE, SIZE);
    glVertex3f(-SIZE, -SIZE, SIZE);
    glTexCoord3f(-SIZE, SIZE, SIZE);
    glVertex3f(-SIZE, SIZE, SIZE);
    glTexCoord3f(SIZE, SIZE, SIZE);
    glVertex3f(SIZE, SIZE, SIZE);
    glEnd();

    // Back face (Negative Z)
    glBegin(GL_QUADS);
    glTexCoord3f(-SIZE, -SIZE, -SIZE);
    glVertex3f(-SIZE, -SIZE, -SIZE);
    glTexCoord3f(SIZE, -SIZE, -SIZE);
    glVertex3f(SIZE, -SIZE, -SIZE);
    glTexCoord3f(SIZE, SIZE, -SIZE);
    glVertex3f(SIZE, SIZE, -SIZE);
    glTexCoord3f(-SIZE, SIZE, -SIZE);
    glVertex3f(-SIZE, SIZE, -SIZE);
    glEnd();

    // Right face (Positive X)
    glBegin(GL_QUADS);
    glTexCoord3f(SIZE, -SIZE, -SIZE);
    glVertex3f(SIZE, -SIZE, -SIZE);
    glTexCoord3f(SIZE, -SIZE, SIZE);
    glVertex3f(SIZE, -SIZE, SIZE);
    glTexCoord3f(SIZE, SIZE, SIZE);
    glVertex3f(SIZE, SIZE, SIZE);
    glTexCoord3f(SIZE, SIZE, -SIZE);
    glVertex3f(SIZE, SIZE, -SIZE);
    glEnd();

    // Left face (Negative X)
    glBegin(GL_QUADS);
    glTexCoord3f(-SIZE, -SIZE, SIZE);
    glVertex3f(-SIZE, -SIZE, SIZE);
    glTexCoord3f(-SIZE, -SIZE, -SIZE);
    glVertex3f(-SIZE, -SIZE, -SIZE);
    glTexCoord3f(-SIZE, SIZE, -SIZE);
    glVertex3f(-SIZE, SIZE, -SIZE);
    glTexCoord3f(-SIZE, SIZE, SIZE);
    glVertex3f(-SIZE, SIZE, SIZE);
    glEnd();

    // Top face (Positive Y)
    glBegin(GL_QUADS);
    glTexCoord3f(SIZE, SIZE, SIZE);
    glVertex3f(SIZE, SIZE, SIZE);
    glTexCoord3f(-SIZE, SIZE, SIZE);
    glVertex3f(-SIZE, SIZE, SIZE);
    glTexCoord3f(-SIZE, SIZE, -SIZE);
    glVertex3f(-SIZE, SIZE, -SIZE);
    glTexCoord3f(SIZE, SIZE, -SIZE);
    glVertex3f(SIZE, SIZE, -SIZE);
    glEnd();

    // Bottom face (Negative Y)
    glBegin(GL_QUADS);
    glTexCoord3f(SIZE, -SIZE, -SIZE);
    glVertex3f(SIZE, -SIZE, -SIZE);
    glTexCoord3f(-SIZE, -SIZE, -SIZE);
    glVertex3f(-SIZE, -SIZE, -SIZE);
    glTexCoord3f(-SIZE, -SIZE, SIZE);
    glVertex3f(-SIZE, -SIZE, SIZE);
    glTexCoord3f(SIZE, -SIZE, SIZE);
    glVertex3f(SIZE, -SIZE, SIZE);
    glEnd();

    glDisable(GL_TEXTURE_CUBE_MAP);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}
