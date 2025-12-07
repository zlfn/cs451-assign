#include "PostProcess.hpp"
#include <iostream>

MotionBlurProcessor::MotionBlurProcessor()
    : width_(0), height_(0), initialized_(false),
      sceneFBO_(0), sceneTexture_(0),
      prevFrameFBO_(0), prevFrameTexture_(0),
      depthRBO_(0), quadVAO_(0), quadVBO_(0) {}

MotionBlurProcessor::~MotionBlurProcessor() {
    if (sceneFBO_) {
        deleteFramebuffer(sceneFBO_, sceneTexture_);
    }
    if (prevFrameFBO_) {
        deleteFramebuffer(prevFrameFBO_, prevFrameTexture_);
    }
    if (depthRBO_) {
        glDeleteRenderbuffers(1, &depthRBO_);
    }
    if (quadVAO_) {
        glDeleteVertexArrays(1, &quadVAO_);
    }
    if (quadVBO_) {
        glDeleteBuffers(1, &quadVBO_);
    }
}

void MotionBlurProcessor::init(int width, int height) {
    width_ = width;
    height_ = height;

    // Create scene framebuffer with depth
    createFramebuffer(sceneFBO_, sceneTexture_, width, height);

    // Attach depth renderbuffer to scene FBO
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO_);
    glGenRenderbuffers(1, &depthRBO_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRBO_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO_);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Scene framebuffer is not complete!" << std::endl;
    }

    // Create previous frame framebuffer (no depth needed)
    createFramebuffer(prevFrameFBO_, prevFrameTexture_, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create screen quad
    createScreenQuad();

    initialized_ = true;
    std::cout << "Motion blur processor initialized (" << width << "x" << height << ")" << std::endl;
}

void MotionBlurProcessor::resize(int width, int height) {
    if (width == width_ && height == height_) return;

    width_ = width;
    height_ = height;

    // Recreate textures with new size
    glBindTexture(GL_TEXTURE_2D, sceneTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

    glBindTexture(GL_TEXTURE_2D, prevFrameTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

    // Recreate depth buffer
    glBindRenderbuffer(GL_RENDERBUFFER, depthRBO_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void MotionBlurProcessor::beginScene() {
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO_);
    glViewport(0, 0, width_, height_);
}

void MotionBlurProcessor::endSceneAndApply(ShaderProgram& motionBlurShader, float blendFactor) {
    // Bind default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width_, height_);

    // Clear and disable depth test for post-process quad
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    // Use motion blur shader
    motionBlurShader.use();

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture_);
    motionBlurShader.setUniform("currentFrame", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, prevFrameTexture_);
    motionBlurShader.setUniform("previousFrame", 1);

    motionBlurShader.setUniform("blendFactor", blendFactor);

    // Draw screen quad
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    // Re-enable depth test
    glEnable(GL_DEPTH_TEST);

    // Copy current frame to previous frame for next iteration
    glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneFBO_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevFrameFBO_);
    glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MotionBlurProcessor::createFramebuffer(GLuint& fbo, GLuint& texture, int width, int height) {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
}

void MotionBlurProcessor::deleteFramebuffer(GLuint& fbo, GLuint& texture) {
    if (texture) {
        glDeleteTextures(1, &texture);
        texture = 0;
    }
    if (fbo) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
}

void MotionBlurProcessor::createScreenQuad() {
    // Screen quad vertices: position (x, y), texCoord (u, v)
    float quadVertices[] = {
        // Position     // TexCoord
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f,  1.0f,   1.0f, 1.0f,
    };

    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &quadVBO_);

    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // TexCoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}
