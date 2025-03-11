#include "InGameHUD.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

InGameHUD::InGameHUD(int screenWidth, int screenHeight, GLuint textureAtlas)
    : screenWidth(screenWidth), screenHeight(screenHeight), textureAtlas(textureAtlas), hudShader("../shaders/Hud.vert", "../shaders/Hud.frag") {
    SetupCrosshair();
}

InGameHUD::~InGameHUD() {
    glDeleteVertexArrays(1, &crosshairVAO);
    glDeleteBuffers(1, &crosshairVBO);
}

void InGameHUD::SetupCrosshair() {
    float size = 32.0f;  // Size in pixels

    // Convert screen-space coordinates to NDC (-1 to 1)
    float x = screenWidth / 2.0f - size / 2.0f;
    float y = screenHeight / 2.0f - size / 2.0f;

    // Texture Atlas UVs (First 16x16 pixels in a 256x256 texture)
    float texSize = 16.0f / 256.0f; // 16x16 portion

    // Adjusted UVs: Since OpenGL's texture coordinate system starts at the bottom-left,
    // we need to map the top-left 16x16 region properly.
    float texX = 0.0f;      // Top-left corner in UV space
    float texY = 1.0f - texSize;  // Move down by 16 pixels

    float crosshairVertices[] = {
        // Positions (NDC)   // Texture Coords (UVs)
        x,          y + size,  0.0f,  texX,        texY + texSize,  // Top-left
        x + size, y + size,  0.0f,  texX + texSize, texY + texSize,  // Top-right
        x + size, y,           0.0f,  texX + texSize, texY,  // Bottom-right
        x,          y,           0.0f,  texX,        texY   // Bottom-left
    };


    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &crosshairVBO);
    glGenBuffers(1, &crosshairEBO);

    glBindVertexArray(crosshairVAO);

    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, crosshairEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position Attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture Coordinate Attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void InGameHUD::RenderCrosshair() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    hudShader.use();

    const glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(screenWidth), 0.0f, static_cast<float>(screenHeight), -1.0f, 1.0f);
    hudShader.setMat4("projection", projection);


    glActiveTexture(GL_TEXTURE2);  // Use GL_TEXTURE2 to avoid conflicts
    glBindTexture(GL_TEXTURE_2D, textureAtlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    hudShader.setInt("texture1", 2);  // Set uniform to slot 2

    glBindVertexArray(crosshairVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}



void InGameHUD::RenderHUD(glm::vec3 playerPosition, glm::vec2 chunkPosition) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text(("Position: " + std::to_string(playerPosition.x) + " " +
                 std::to_string(playerPosition.y) + " " +
                 std::to_string(playerPosition.z)).c_str());
    ImGui::Text(("Chunk: " + std::to_string(static_cast<int>(chunkPosition.x)) + " " +
                 std::to_string(static_cast<int>(chunkPosition.y))).c_str());
    ImGui::End();

    RenderCrosshair();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
