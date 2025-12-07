#pragma once

#include <GL/glew.h>
#include <memory>
#include "graphics.hpp"

class MotionBlurProcessor {
public:
    MotionBlurProcessor();
    ~MotionBlurProcessor();

    // Initialize with window dimensions
    void init(int width, int height);

    // Resize framebuffers when window size changes
    void resize(int width, int height);

    // Bind scene framebuffer - render scene to this
    void beginScene();

    // Unbind scene framebuffer and apply motion blur
    void endSceneAndApply(ShaderProgram& motionBlurShader, float blendFactor);

    bool isInitialized() const { return initialized_; }

private:
    void createFramebuffer(GLuint& fbo, GLuint& texture, int width, int height);
    void deleteFramebuffer(GLuint& fbo, GLuint& texture);
    void createScreenQuad();

    int width_;
    int height_;
    bool initialized_;

    // Ping-pong framebuffers for motion blur
    GLuint sceneFBO_;           // Current frame renders here
    GLuint sceneTexture_;       // Current frame texture

    GLuint prevFrameFBO_;       // Previous frame stored here
    GLuint prevFrameTexture_;   // Previous frame texture

    GLuint depthRBO_;           // Depth renderbuffer for scene

    // Screen-space quad for post-processing
    GLuint quadVAO_;
    GLuint quadVBO_;
};
