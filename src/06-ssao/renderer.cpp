#include "renderer.h"
#include "imgui.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

bool Renderer::init() {
    // load shader (copied to build/shaders by CMake post build)
    if (!shader_.loadFromFiles("shaders/simple.vert", "shaders/simple.frag")) {
        std::cerr << "Failed to load shader" << std::endl;
        return false;
    }

    if (!gbufferShader_.loadFromFiles("shaders/gbuffer.vert", "shaders/gbuffer.frag")) {
        std::cerr << "Failed to load gbuffer shader" << std::endl;
        return false;
    }
    if (!debugShader_.loadFromFiles("shaders/quad.vert", "shaders/debug_display.frag")) {
        std::cerr << "Failed to load debug shader" << std::endl;
        return false;
    }

    if (!lightingShader_.loadFromFiles("shaders/quad.vert", "shaders/lighting.frag")) {
        std::cerr << "Failed to load lighting shader" << std::endl;
        return false;
    }

    if (!ssaoShader_.loadFromFiles("shaders/quad.vert", "shaders/ssao.frag")) {
        std::cerr << "Failed to load SSAO shader" << std::endl;
        return false;
    }

    if (!ssaoBlurShader_.loadFromFiles("shaders/quad.vert", "shaders/ssao_blur.frag")) {
        std::cerr << "Failed to load SSAO blur shader" << std::endl;
        return false;
    }

    initSSAO();

    // initialize G-buffer with a default size (will resize later if needed)
    if (!gbuffer_.init(1280, 720)) {
        std::cerr << "Failed to init GBuffer" << std::endl;
        return false;
    }

    // Enable face culling for better performance and correct rendering
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    createCube();
    createQuad();

    if (!initScene()) {
        std::cerr << "Failed to initialize scene" << std::endl;
        return false;
    }

    return true;
}

void Renderer::createCube() {
    float vertices[] = {
        // positions          // normals           // texcoords
        // Front face
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
         1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,

        // Back face
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
        -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
         1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,

        // Left face
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

        // Right face
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
         1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
         1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,

        // Bottom face
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
        -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,

        // Top face
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
        -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
         1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
         1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
         1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f
    };

    if (cubeVAO_) return;
    glGenVertexArrays(1, &cubeVAO_);
    glGenBuffers(1, &cubeVBO_);
    glBindVertexArray(cubeVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);
}

void Renderer::createQuad() {
    float quadVertices[] = {
        // pos   // uv
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };
    if (quadVAO_) return;
    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &quadVBO_);
    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

void Renderer::render(float time, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
    glm::mat4 view = viewMatrix;
    glm::mat4 proj = projMatrix;

    // Update dynamic lighting
    updateDynamicLighting(time);

    // 1) Geometry pass: render scene into G-buffer
    gbuffer_.bindForWriting();
    glViewport(0,0,1280,720);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gbufferShader_.use();
    GLuint pid = gbufferShader_.id();
    GLint loc;
    loc = glGetUniformLocation(pid, "uView"); if (loc>=0) glUniformMatrix4fv(loc, 1, GL_FALSE, &view[0][0]);
    loc = glGetUniformLocation(pid, "uProj"); if (loc>=0) glUniformMatrix4fv(loc, 1, GL_FALSE, &proj[0][0]);

    // Render scene geometry
    for (size_t i = 0; i < sceneTransforms_.size(); ++i) {
        const auto& transform = sceneTransforms_[i];
        const auto& color = sceneColors_[i];

        loc = glGetUniformLocation(pid, "uModel"); if (loc>=0) glUniformMatrix4fv(loc, 1, GL_FALSE, &transform[0][0]);
        glUniform1i(glGetUniformLocation(pid, "uHasAlbedoTex"), 0); // No textures for now

        // Set color uniform (we'll modify gbuffer shader to use this)
        glUniform3fv(glGetUniformLocation(pid, "uAlbedoColor"), 1, &color[0]);

        glBindVertexArray(cubeVAO_);
        glDrawArrays(GL_TRIANGLES, 0, 36); // Full cube
        glBindVertexArray(0);
    }

    gbuffer_.unbind();

    // 2) SSAO pass
    renderSSAO(view, proj);
    blurSSAO();

    // 3) Lighting pass or debug display
    glViewport(0,0,1280,720);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (debugMode_ == 3) {
        // Render lighting
        lightingShader_.use();
        GLuint lp = lightingShader_.id();

        // G-buffer textures
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, gbuffer_.getAlbedoTex());
        glUniform1i(glGetUniformLocation(lp, "uAlbedoTex"), 0);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gbuffer_.getNormalTex());
        glUniform1i(glGetUniformLocation(lp, "uNormalTex"), 1);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, gbuffer_.getDepthTex());
        glUniform1i(glGetUniformLocation(lp, "uDepthTex"), 2);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, ssaoBlurTex_);
        glUniform1i(glGetUniformLocation(lp, "uSSAOTex"), 3);

        // Camera matrices
        glm::mat4 invView = glm::inverse(view);
        glm::mat4 invProj = glm::inverse(proj);
        glUniformMatrix4fv(glGetUniformLocation(lp, "uView"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(lp, "uProj"), 1, GL_FALSE, &proj[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(lp, "uInvView"), 1, GL_FALSE, &invView[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(lp, "uInvProj"), 1, GL_FALSE, &invProj[0][0]);
        glUniform1f(glGetUniformLocation(lp, "uNear"), 0.1f);
        glUniform1f(glGetUniformLocation(lp, "uFar"), 100.0f);

        // Directional light
        glUniform3fv(glGetUniformLocation(lp, "uDirectionalLightDir"), 1, &directionalLight_.direction[0]);
        glUniform3fv(glGetUniformLocation(lp, "uDirectionalLightColor"), 1, &directionalLight_.color[0]);
        glUniform1f(glGetUniformLocation(lp, "uDirectionalLightIntensity"), directionalLight_.intensity);

        // Point lights (up to 8)
        int numLights = std::min(8, (int)pointLights_.size());
        glUniform1i(glGetUniformLocation(lp, "uNumPointLights"), numLights);

        for (int i = 0; i < numLights; ++i) {
            std::string posName = "uPointLightPositions[" + std::to_string(i) + "]";
            std::string colorName = "uPointLightColors[" + std::to_string(i) + "]";
            std::string intensityName = "uPointLightIntensities[" + std::to_string(i) + "]";
            std::string radiusName = "uPointLightRadii[" + std::to_string(i) + "]";

            glUniform3fv(glGetUniformLocation(lp, posName.c_str()), 1, &pointLights_[i].position[0]);
            glUniform3fv(glGetUniformLocation(lp, colorName.c_str()), 1, &pointLights_[i].color[0]);
            glUniform1f(glGetUniformLocation(lp, intensityName.c_str()), pointLights_[i].intensity);
            glUniform1f(glGetUniformLocation(lp, radiusName.c_str()), pointLights_[i].radius);
        }

        renderQuad();
    } else if (debugMode_ == 4) {
        // Debug display of SSAO
        debugShader_.use();
        GLuint dp = debugShader_.id();
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, ssaoBlurTex_);
        glUniform1i(glGetUniformLocation(dp, "uAlbedo"), 0);
        glUniform1i(glGetUniformLocation(dp, "uMode"), 0); // Display as albedo (red channel)
        renderQuad();
    } else {
        // Debug display of G-buffer textures
    debugShader_.use();
    GLuint dp = debugShader_.id();
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, gbuffer_.getAlbedoTex());
    glUniform1i(glGetUniformLocation(dp, "uAlbedo"), 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gbuffer_.getNormalTex());
    glUniform1i(glGetUniformLocation(dp, "uNormal"), 1);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, gbuffer_.getDepthTex());
    glUniform1i(glGetUniformLocation(dp, "uDepth"), 2);
    glUniform1i(glGetUniformLocation(dp, "uMode"), debugMode_);
    glUniform1f(glGetUniformLocation(dp, "uNear"), 0.1f);
    glUniform1f(glGetUniformLocation(dp, "uFar"), 100.0f);

    renderQuad();
    }

    // debug UI is rendered separately (must be called between NewFrame() and Render())

}

void Renderer::renderQuad() {
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

bool Renderer::initScene() {
    // Create a simple gunfight scene with walls, ground, and cover objects

    // Ground plane
    glm::mat4 groundTransform = glm::scale(glm::mat4(1.0f), glm::vec3(20.0f, 0.1f, 20.0f));
    groundTransform = glm::translate(groundTransform, glm::vec3(0.0f, -5.0f, 0.0f));
    sceneTransforms_.push_back(groundTransform);
    sceneColors_.push_back(glm::vec3(0.3f, 0.3f, 0.3f)); // Dark gray ground

    // Walls
    // Left wall
    glm::mat4 leftWallTransform = glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 5.0f, 20.0f));
    leftWallTransform = glm::translate(leftWallTransform, glm::vec3(-10.0f, 0.0f, 0.0f));
    sceneTransforms_.push_back(leftWallTransform);
    sceneColors_.push_back(glm::vec3(0.8f, 0.8f, 0.8f)); // Light gray walls

    // Right wall
    glm::mat4 rightWallTransform = glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 5.0f, 20.0f));
    rightWallTransform = glm::translate(rightWallTransform, glm::vec3(10.0f, 0.0f, 0.0f));
    sceneTransforms_.push_back(rightWallTransform);
    sceneColors_.push_back(glm::vec3(0.8f, 0.8f, 0.8f));

    // Back wall
    glm::mat4 backWallTransform = glm::scale(glm::mat4(1.0f), glm::vec3(20.0f, 5.0f, 0.2f));
    backWallTransform = glm::translate(backWallTransform, glm::vec3(0.0f, 0.0f, -10.0f));
    sceneTransforms_.push_back(backWallTransform);
    sceneColors_.push_back(glm::vec3(0.8f, 0.8f, 0.8f));

    // Front wall (with opening)
    glm::mat4 frontLeftWallTransform = glm::scale(glm::mat4(1.0f), glm::vec3(7.0f, 5.0f, 0.2f));
    frontLeftWallTransform = glm::translate(frontLeftWallTransform, glm::vec3(-6.5f, 0.0f, 10.0f));
    sceneTransforms_.push_back(frontLeftWallTransform);
    sceneColors_.push_back(glm::vec3(0.8f, 0.8f, 0.8f));

    glm::mat4 frontRightWallTransform = glm::scale(glm::mat4(1.0f), glm::vec3(7.0f, 5.0f, 0.2f));
    frontRightWallTransform = glm::translate(frontRightWallTransform, glm::vec3(6.5f, 0.0f, 10.0f));
    sceneTransforms_.push_back(frontRightWallTransform);
    sceneColors_.push_back(glm::vec3(0.8f, 0.8f, 0.8f));

    // Cover objects (crates, barrels, etc.)
    // Large crate in center
    glm::mat4 crateTransform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
    crateTransform = glm::translate(crateTransform, glm::vec3(0.0f, -1.0f, 0.0f));
    sceneTransforms_.push_back(crateTransform);
    sceneColors_.push_back(glm::vec3(0.6f, 0.4f, 0.2f)); // Brown crate

    // Small barrels on sides
    glm::mat4 barrel1Transform = glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 1.5f, 0.8f));
    barrel1Transform = glm::translate(barrel1Transform, glm::vec3(-3.0f, -1.75f, -3.0f));
    sceneTransforms_.push_back(barrel1Transform);
    sceneColors_.push_back(glm::vec3(0.3f, 0.3f, 0.3f)); // Dark metal

    glm::mat4 barrel2Transform = glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 1.5f, 0.8f));
    barrel2Transform = glm::translate(barrel2Transform, glm::vec3(3.0f, -1.75f, 3.0f));
    sceneTransforms_.push_back(barrel2Transform);
    sceneColors_.push_back(glm::vec3(0.3f, 0.3f, 0.3f));

    // Add more scene elements for a richer gunfight scene
    // Corner pillars
    glm::mat4 pillar1Transform = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 4.0f, 0.5f));
    pillar1Transform = glm::translate(pillar1Transform, glm::vec3(-8.0f, 1.0f, -8.0f));
    sceneTransforms_.push_back(pillar1Transform);
    sceneColors_.push_back(glm::vec3(0.7f, 0.7f, 0.7f));

    glm::mat4 pillar2Transform = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 4.0f, 0.5f));
    pillar2Transform = glm::translate(pillar2Transform, glm::vec3(8.0f, 1.0f, -8.0f));
    sceneTransforms_.push_back(pillar2Transform);
    sceneColors_.push_back(glm::vec3(0.7f, 0.7f, 0.7f));

    glm::mat4 pillar3Transform = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 4.0f, 0.5f));
    pillar3Transform = glm::translate(pillar3Transform, glm::vec3(-8.0f, 1.0f, 8.0f));
    sceneTransforms_.push_back(pillar3Transform);
    sceneColors_.push_back(glm::vec3(0.7f, 0.7f, 0.7f));

    glm::mat4 pillar4Transform = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 4.0f, 0.5f));
    pillar4Transform = glm::translate(pillar4Transform, glm::vec3(8.0f, 1.0f, 8.0f));
    sceneTransforms_.push_back(pillar4Transform);
    sceneColors_.push_back(glm::vec3(0.7f, 0.7f, 0.7f));

    // Low walls for cover
    glm::mat4 lowWall1Transform = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 1.5f, 0.3f));
    lowWall1Transform = glm::translate(lowWall1Transform, glm::vec3(-4.0f, -2.25f, -2.0f));
    sceneTransforms_.push_back(lowWall1Transform);
    sceneColors_.push_back(glm::vec3(0.6f, 0.6f, 0.6f));

    glm::mat4 lowWall2Transform = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 1.5f, 0.3f));
    lowWall2Transform = glm::translate(lowWall2Transform, glm::vec3(4.0f, -2.25f, 2.0f));
    sceneTransforms_.push_back(lowWall2Transform);
    sceneColors_.push_back(glm::vec3(0.6f, 0.6f, 0.6f));

    // Scattered debris/crates
    glm::mat4 debris1Transform = glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 0.8f, 0.8f));
    debris1Transform = glm::translate(debris1Transform, glm::vec3(-2.0f, -2.6f, -4.0f));
    sceneTransforms_.push_back(debris1Transform);
    sceneColors_.push_back(glm::vec3(0.4f, 0.2f, 0.1f));

    glm::mat4 debris2Transform = glm::scale(glm::mat4(1.0f), glm::vec3(0.6f, 1.2f, 0.6f));
    debris2Transform = glm::translate(debris2Transform, glm::vec3(1.5f, -2.4f, -3.5f));
    sceneTransforms_.push_back(debris2Transform);
    sceneColors_.push_back(glm::vec3(0.4f, 0.2f, 0.1f));

    glm::mat4 debris3Transform = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 0.4f, 1.0f));
    debris3Transform = glm::translate(debris3Transform, glm::vec3(0.0f, -3.3f, 4.0f));
    sceneTransforms_.push_back(debris3Transform);
    sceneColors_.push_back(glm::vec3(0.4f, 0.2f, 0.1f));

    std::cout << "Gunfight scene initialized with " << sceneTransforms_.size() << " objects" << std::endl;

    setupLighting();

    return true;
}

void Renderer::setupLighting() {
    // Setup directional light (sun/moon light)
    directionalLight_.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.5f));
    directionalLight_.color = glm::vec3(0.8f, 0.9f, 1.0f); // Slightly blue-tinted sunlight
    directionalLight_.intensity = 0.6f;

    // Setup point lights for dramatic gunfight lighting
    pointLights_.clear();

    // Main overhead light (dim ambient)
    PointLight overheadLight;
    overheadLight.position = glm::vec3(0.0f, 8.0f, 0.0f);
    overheadLight.color = glm::vec3(0.7f, 0.7f, 0.8f);
    overheadLight.intensity = 0.3f;
    overheadLight.radius = 25.0f;
    pointLights_.push_back(overheadLight);

    // Tactical lights in corners
    PointLight cornerLight1;
    cornerLight1.position = glm::vec3(-6.0f, 3.0f, -6.0f);
    cornerLight1.color = glm::vec3(1.0f, 0.8f, 0.6f); // Warm light
    cornerLight1.intensity = 1.2f;
    cornerLight1.radius = 8.0f;
    pointLights_.push_back(cornerLight1);

    PointLight cornerLight2;
    cornerLight2.position = glm::vec3(6.0f, 3.0f, 6.0f);
    cornerLight2.color = glm::vec3(0.6f, 0.8f, 1.0f); // Cool light
    cornerLight2.intensity = 1.0f;
    cornerLight2.radius = 8.0f;
    pointLights_.push_back(cornerLight2);

    // Dynamic lights that will be updated each frame
    PointLight flashLight1;
    flashLight1.position = glm::vec3(-2.0f, 2.0f, 2.0f);
    flashLight1.color = glm::vec3(1.0f, 1.0f, 0.8f); // Bright white-yellow
    flashLight1.intensity = 2.0f;
    flashLight1.radius = 6.0f;
    pointLights_.push_back(flashLight1);

    PointLight flashLight2;
    flashLight2.position = glm::vec3(2.0f, 1.5f, -2.0f);
    flashLight2.color = glm::vec3(1.0f, 0.3f, 0.1f); // Red-orange muzzle flash
    flashLight2.intensity = 1.5f;
    flashLight2.radius = 5.0f;
    pointLights_.push_back(flashLight2);

    // Add some flickering emergency lights
    PointLight emergencyLight1;
    emergencyLight1.position = glm::vec3(-7.0f, 3.0f, -7.0f);
    emergencyLight1.color = glm::vec3(1.0f, 0.0f, 0.0f); // Red emergency light
    emergencyLight1.intensity = 1.0f;
    emergencyLight1.radius = 4.0f;
    pointLights_.push_back(emergencyLight1);

    PointLight emergencyLight2;
    emergencyLight2.position = glm::vec3(7.0f, 3.0f, 7.0f);
    emergencyLight2.color = glm::vec3(0.0f, 0.0f, 1.0f); // Blue emergency light
    emergencyLight2.intensity = 1.0f;
    emergencyLight2.radius = 4.0f;
    pointLights_.push_back(emergencyLight2);

    std::cout << "Lighting system initialized with " << pointLights_.size() << " point lights" << std::endl;
}

void Renderer::renderLighting(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
    // This function will be called after G-buffer pass to compute lighting
    // For now, we'll implement it in the next step when we create the lighting shader
}

void Renderer::initSSAO() {
    // Create SSAO framebuffer and texture
    glGenFramebuffers(1, &ssaoFBO_);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO_);

    glGenTextures(1, &ssaoTex_);
    glBindTexture(GL_TEXTURE_2D, ssaoTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1280, 720, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTex_, 0);

    // Create blur framebuffer and texture
    glGenFramebuffers(1, &ssaoBlurFBO_);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO_);

    glGenTextures(1, &ssaoBlurTex_);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1280, 720, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurTex_, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Generate random kernel samples for SSAO
    std::vector<glm::vec3> ssaoKernel;
    for (unsigned int i = 0; i < 64; ++i) {
        glm::vec3 sample(
            (rand() % 2000 - 1000.0f) / 1000.0f,
            (rand() % 2000 - 1000.0f) / 1000.0f,
            rand() % 1000 / 1000.0f
        );
        sample = glm::normalize(sample);
        sample *= (rand() % 1000 / 1000.0f);
        float scale = (float)i / 64.0f;
        scale = 0.1f + scale * scale * 0.9f; // lerp(0.1, 1.0, scale*scale)
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    // Generate noise texture for random rotation
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; ++i) {
        glm::vec3 noise(
            (rand() % 2000 - 1000.0f) / 1000.0f,
            (rand() % 2000 - 1000.0f) / 1000.0f,
            0.0f
        );
        ssaoNoise.push_back(noise);
    }

    glGenTextures(1, &noiseTex_);
    glBindTexture(GL_TEXTURE_2D, noiseTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    std::cout << "SSAO system initialized" << std::endl;
}

void Renderer::updateDynamicLighting(float time) {
    if (pointLights_.size() >= 5) { // We added 6 lights total
        // Flashlight 1: Pulsing effect
        pointLights_[3].intensity = 2.0f + sin(time * 3.0f) * 0.5f;

        // Flashlight 2: Muzzle flash simulation (random bursts)
        float flashPattern = sin(time * 10.0f) * cos(time * 7.0f);
        pointLights_[4].intensity = 1.5f + flashPattern * 0.8f;
        pointLights_[4].color = glm::vec3(1.0f, 0.3f + flashPattern * 0.2f, 0.1f);

        // Emergency light 1: Red flashing
        float redFlash = (sin(time * 2.0f) > 0.0f) ? 1.0f : 0.3f;
        pointLights_[5].intensity = redFlash * 1.5f;

        // Emergency light 2: Blue flashing (opposite phase)
        float blueFlash = (sin(time * 2.0f + 3.14159f) > 0.0f) ? 1.0f : 0.3f;
        pointLights_[6].intensity = blueFlash * 1.5f;
    }
}

void Renderer::renderSSAO(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO_);
    glClear(GL_COLOR_BUFFER_BIT);

    ssaoShader_.use();
    GLuint sp = ssaoShader_.id();

    // Depth texture only for simplified SSAO
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, gbuffer_.getDepthTex());
    glUniform1i(glGetUniformLocation(sp, "uDepthTex"), 0);

    // SSAO parameters
    glUniform1f(glGetUniformLocation(sp, "uNear"), 0.1f);
    glUniform1f(glGetUniformLocation(sp, "uFar"), 100.0f);

    renderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::blurSSAO() {
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO_);
    glClear(GL_COLOR_BUFFER_BIT);

    ssaoBlurShader_.use();
    GLuint bp = ssaoBlurShader_.id();

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, ssaoTex_);
    glUniform1i(glGetUniformLocation(bp, "uSSAOInput"), 0);

    renderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::showDebugUI() {
    ImGui::Begin("G-buffer Debug");
    ImGui::RadioButton("Albedo", &debugMode_, 0); ImGui::SameLine();
    ImGui::RadioButton("Normal", &debugMode_, 1); ImGui::SameLine();
    ImGui::RadioButton("Depth", &debugMode_, 2); ImGui::SameLine();
    ImGui::RadioButton("Lighting", &debugMode_, 3); ImGui::SameLine();
    ImGui::RadioButton("SSAO", &debugMode_, 4);
    ImGui::End();
}
