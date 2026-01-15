#include "gbuffer.h"
#include <iostream>

GBuffer::~GBuffer() { cleanup(); }

void GBuffer::cleanup() {
    if (texAlbedo_) { glDeleteTextures(1, &texAlbedo_); texAlbedo_ = 0; }
    if (texNormal_) { glDeleteTextures(1, &texNormal_); texNormal_ = 0; }
    if (texDepth_) { glDeleteTextures(1, &texDepth_); texDepth_ = 0; }
    if (fbo_) { glDeleteFramebuffers(1, &fbo_); fbo_ = 0; }
}

bool GBuffer::init(int width, int height) {
    cleanup();
    width_ = width; height_ = height;

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    // Albedo (RGB8)
    glGenTextures(1, &texAlbedo_);
    glBindTexture(GL_TEXTURE_2D, texAlbedo_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width_, height_, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texAlbedo_, 0);

    // Normal (RGBA16F)
    glGenTextures(1, &texNormal_);
    glBindTexture(GL_TEXTURE_2D, texNormal_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width_, height_, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texNormal_, 0);

    // Depth
    glGenTextures(1, &texDepth_);
    glBindTexture(GL_TEXTURE_2D, texDepth_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width_, height_, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texDepth_, 0);

    GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "GBuffer incomplete" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void GBuffer::resize(int width, int height) {
    if (width == width_ && height == height_) return;
    init(width, height);
}

void GBuffer::bindForWriting() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
}

void GBuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
