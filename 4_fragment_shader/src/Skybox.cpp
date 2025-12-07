#include "Skybox.hpp"
#include "base.hpp"
#include "stb_image.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

Skybox::Skybox() : textureID_(0), loaded_(false) {
    // Don't create mesh and shader here - OpenGL context may not be ready yet
    // They will be created lazily in draw() when needed
}

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

void Skybox::createCubeMesh() {
    // Skybox cube vertices (positions only, texture coords will be the positions themselves)
    const float SIZE = 14.0f;
    float vertices[] = {
        // positions (x, y, z)
        // Front face
        -SIZE, -SIZE,  SIZE,
         SIZE, -SIZE,  SIZE,
         SIZE,  SIZE,  SIZE,
         SIZE,  SIZE,  SIZE,
        -SIZE,  SIZE,  SIZE,
        -SIZE, -SIZE,  SIZE,

        // Back face
        -SIZE, -SIZE, -SIZE,
        -SIZE,  SIZE, -SIZE,
         SIZE,  SIZE, -SIZE,
         SIZE,  SIZE, -SIZE,
         SIZE, -SIZE, -SIZE,
        -SIZE, -SIZE, -SIZE,

        // Left face
        -SIZE,  SIZE,  SIZE,
        -SIZE,  SIZE, -SIZE,
        -SIZE, -SIZE, -SIZE,
        -SIZE, -SIZE, -SIZE,
        -SIZE, -SIZE,  SIZE,
        -SIZE,  SIZE,  SIZE,

        // Right face
         SIZE,  SIZE,  SIZE,
         SIZE, -SIZE,  SIZE,
         SIZE, -SIZE, -SIZE,
         SIZE, -SIZE, -SIZE,
         SIZE,  SIZE, -SIZE,
         SIZE,  SIZE,  SIZE,

        // Top face
        -SIZE,  SIZE, -SIZE,
        -SIZE,  SIZE,  SIZE,
         SIZE,  SIZE,  SIZE,
         SIZE,  SIZE,  SIZE,
         SIZE,  SIZE, -SIZE,
        -SIZE,  SIZE, -SIZE,

        // Bottom face
        -SIZE, -SIZE, -SIZE,
         SIZE, -SIZE, -SIZE,
         SIZE, -SIZE,  SIZE,
         SIZE, -SIZE,  SIZE,
        -SIZE, -SIZE,  SIZE,
        -SIZE, -SIZE, -SIZE
    };

    cubeMesh_ = std::make_unique<Mesh>();
    cubeMesh_->setData(vertices, sizeof(vertices), GL_STATIC_DRAW);
    cubeMesh_->setAttribute(0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);
    cubeMesh_->setDrawMode(GL_TRIANGLES, 36);
}

void Skybox::createShader() {
    // Skybox vertex shader
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;

        uniform mat4 projection;
        uniform mat4 view;

        out vec3 TexCoords;

        void main() {
            TexCoords = position;
            vec4 pos = projection * view * vec4(position, 1.0);
            gl_Position = pos.xyww; // Trick to make depth always 1.0
        }
    )";

    // Skybox fragment shader
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 TexCoords;
        out vec4 FragColor;

        uniform samplerCube skybox;

        void main() {
            FragColor = texture(skybox, TexCoords);
        }
    )";

    try {
        skyboxShader_ = std::make_unique<ShaderProgram>();

        Shader vertShader = Shader::fromSource(Shader::Type::VERTEX, vertexShaderSource);
        Shader fragShader = Shader::fromSource(Shader::Type::FRAGMENT, fragmentShaderSource);

        skyboxShader_->attachShader(vertShader);
        skyboxShader_->attachShader(fragShader);
        skyboxShader_->link();
    } catch (const std::exception& e) {
        std::cerr << "Failed to create skybox shader: " << e.what() << '\n';
        skyboxShader_.reset();
    }
}

void Skybox::draw(const GameState & /*gameState*/) {
    if (!loaded_)
        return;

    // Lazy initialization - create mesh and shader on first draw
    if (!cubeMesh_) {
        createCubeMesh();
    }
    if (!skyboxShader_) {
        createShader();
    }

    if (!cubeMesh_ || !skyboxShader_)
        return;

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    skyboxShader_->use();

    // Bind cubemap texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID_);
    glUniform1i(glGetUniformLocation(skyboxShader_->getId(), "skybox"), 0);

    // Get matrices from stacks
    glm::mat4 projection = projectionStack.getTopMatrix();
    glm::mat4 view = modelViewStack.getTopMatrix();

    // Remove translation from view matrix (keep only rotation)
    view[3][0] = 0.0f;
    view[3][1] = 0.0f;
    view[3][2] = 0.0f;

    skyboxShader_->setUniform("projection", projection);
    skyboxShader_->setUniform("view", view);

    // Draw skybox
    cubeMesh_->bind();
    glDrawArrays(cubeMesh_->getMode(), cubeMesh_->getFirst(), cubeMesh_->getCount());

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}
