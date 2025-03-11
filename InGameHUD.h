#ifndef INGAMEHUD_H
#define INGAMEHUD_H

#include "shader.h"
#include <glm/glm.hpp>

class InGameHUD {
public:
    InGameHUD(int screenWidth, int screenHeight, GLuint textureAtlas);
    ~InGameHUD();

    void RenderHUD(glm::vec3 playerPosition, glm::vec2 chunkPosition);

private:
    int screenWidth, screenHeight;
    GLuint crosshairVAO{}, crosshairVBO{}, crosshairEBO{};
    Shader hudShader;
    GLuint textureAtlas;

    void SetupCrosshair();
    void RenderCrosshair();
};

#endif // INGAMEHUD_H
