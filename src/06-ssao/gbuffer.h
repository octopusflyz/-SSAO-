#pragma once

#include <GL/glew.h>

class GBuffer {
public:
    GBuffer() = default;
    ~GBuffer();

    bool init(int width, int height);
    void resize(int width, int height);

    void bindForWriting();
    void unbind();

    GLuint getAlbedoTex() const { return texAlbedo_; }
    GLuint getNormalTex() const { return texNormal_; }
    GLuint getDepthTex() const { return texDepth_; }

private:
    int width_ = 0, height_ = 0;
    GLuint fbo_ = 0;
    GLuint texAlbedo_ = 0;
    GLuint texNormal_ = 0;
    GLuint texDepth_ = 0;

    void cleanup();
};
