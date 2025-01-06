#include <cmath>
#include <fstream>
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shader.h"
#include "camera.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "block.cpp"
#include "PerlinNoise.hpp"

constexpr unsigned int SCR_WIDTH = 1280;
constexpr unsigned int SCR_HEIGHT = 768;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float yaw, pitch, fov = 90;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

struct Plane {
    glm::vec3 normal;
    float distance;
};

std::array<Plane, 6> extractFrustumPlanes(const glm::mat4& viewProjectionMatrix) {
    std::array<Plane, 6> planes;

    // Left
    planes[0].normal.x = viewProjectionMatrix[0][3] + viewProjectionMatrix[0][0];
    planes[0].normal.y = viewProjectionMatrix[1][3] + viewProjectionMatrix[1][0];
    planes[0].normal.z = viewProjectionMatrix[2][3] + viewProjectionMatrix[2][0];
    planes[0].distance = viewProjectionMatrix[3][3] + viewProjectionMatrix[3][0];

    // Right
    planes[1].normal.x = viewProjectionMatrix[0][3] - viewProjectionMatrix[0][0];
    planes[1].normal.y = viewProjectionMatrix[1][3] - viewProjectionMatrix[1][0];
    planes[1].normal.z = viewProjectionMatrix[2][3] - viewProjectionMatrix[2][0];
    planes[1].distance = viewProjectionMatrix[3][3] - viewProjectionMatrix[3][0];

    // Bottom
    planes[2].normal.x = viewProjectionMatrix[0][3] + viewProjectionMatrix[0][1];
    planes[2].normal.y = viewProjectionMatrix[1][3] + viewProjectionMatrix[1][1];
    planes[2].normal.z = viewProjectionMatrix[2][3] + viewProjectionMatrix[2][1];
    planes[2].distance = viewProjectionMatrix[3][3] + viewProjectionMatrix[3][1];

    // Top
    planes[3].normal.x = viewProjectionMatrix[0][3] - viewProjectionMatrix[0][1];
    planes[3].normal.y = viewProjectionMatrix[1][3] - viewProjectionMatrix[1][1];
    planes[3].normal.z = viewProjectionMatrix[2][3] - viewProjectionMatrix[2][1];
    planes[3].distance = viewProjectionMatrix[3][3] - viewProjectionMatrix[3][1];

    // Near
    planes[4].normal.x = viewProjectionMatrix[0][3] + viewProjectionMatrix[0][2];
    planes[4].normal.y = viewProjectionMatrix[1][3] + viewProjectionMatrix[1][2];
    planes[4].normal.z = viewProjectionMatrix[2][3] + viewProjectionMatrix[2][2];
    planes[4].distance = viewProjectionMatrix[3][3] + viewProjectionMatrix[3][2];

    // Far
    planes[5].normal.x = viewProjectionMatrix[0][3] - viewProjectionMatrix[0][2];
    planes[5].normal.y = viewProjectionMatrix[1][3] - viewProjectionMatrix[1][2];
    planes[5].normal.z = viewProjectionMatrix[2][3] - viewProjectionMatrix[2][2];
    planes[5].distance = viewProjectionMatrix[3][3] - viewProjectionMatrix[3][2];

    // Normalize the planes
    for (auto& plane : planes) {
        float length = glm::length(plane.normal);
        plane.normal /= length;
        plane.distance /= length;
    }

    return planes;
}

bool isBoxInFrustum(const std::array<Plane, 6>& planes, const glm::vec3& min, const glm::vec3& max) {
    for (const auto& plane : planes) {
        glm::vec3 positiveVertex = min;
        glm::vec3 negativeVertex = max;

        if (plane.normal.x >= 0) {
            positiveVertex.x = max.x;
            negativeVertex.x = min.x;
        }
        if (plane.normal.y >= 0) {
            positiveVertex.y = max.y;
            negativeVertex.y = min.y;
        }
        if (plane.normal.z >= 0) {
            positiveVertex.z = max.z;
            negativeVertex.z = min.z;
        }

        if (glm::dot(plane.normal, positiveVertex) + plane.distance < -1.0) {
            return false;
        }
    }
    return true;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
}


void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    auto xpos = static_cast<float>(xposIn);
    auto ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

glm::vec4 getPixelColor(const std::string& filePath, int x, int y) {
    int width, height, nrChannels;

    // Load the image file
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 4); // Force RGBA (4 channels)
    if (!data) {
        std::cerr << "Failed to load image: " << filePath << std::endl;
        return glm::vec4(0.0f); // Return black if the file fails to load
    }

    // Ensure the coordinates are within bounds
    if (x < 0 || x >= width || y < 0 || y >= height) {
        std::cerr << "Coordinates out of bounds: (" << x << ", " << y << ")" << std::endl;
        stbi_image_free(data);
        return glm::vec4(0.0f); // Return black if coordinates are invalid
    }

    // Calculate the index of the pixel in the data array
    int index = (y * width + x) * 4; // Multiply by 4 for RGBA

    // Extract the RGBA components
    unsigned char r = data[index];
    unsigned char g = data[index + 1];
    unsigned char b = data[index + 2];
    unsigned char a = data[index + 3];

    // Free the image memory
    stbi_image_free(data);

    // Convert the components to a normalized float vector (0.0 - 1.0)
    return glm::vec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Gay", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    float vertices[] = {
        // Front face
        -0.5f, -0.5f, -0.5f,  0.0f, 0.9375f, // Bottom-left
         0.5f, -0.5f, -0.5f,  0.0625f, 0.9375f, // Bottom-right
         0.5f,  0.5f, -0.5f,  0.0625f, 1.0f, // Top-right
         0.5f,  0.5f, -0.5f,  0.0625f, 1.0f, // Top-right
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // Top-left
        -0.5f, -0.5f, -0.5f,  0.0f, 0.9375f, // Bottom-left

        // Back face
        -0.5f, -0.5f,  0.5f,  0.0f, 0.9375f, // Bottom-left
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // Top-left
         0.5f,  0.5f,  0.5f,  0.0625f, 1.0f, // Top-right
         0.5f,  0.5f,  0.5f,  0.0625f, 1.0f, // Top-right
         0.5f, -0.5f,  0.5f,  0.0625f, 0.9375f, // Bottom-right
        -0.5f, -0.5f,  0.5f,  0.0f, 0.9375f, // Bottom-left

        // Left face
        -0.5f, -0.5f, -0.5f,  0.0f, 0.9375f, // Bottom-left
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // Top-left
        -0.5f,  0.5f,  0.5f,  0.0625f, 1.0f, // Top-right
        -0.5f,  0.5f,  0.5f,  0.0625f, 1.0f, // Top-right
        -0.5f, -0.5f,  0.5f,  0.0625f, 0.9375f, // Bottom-right
        -0.5f, -0.5f, -0.5f,  0.0f, 0.9375f, // Bottom-left

        // Right face
         0.5f, -0.5f, -0.5f,  0.0f, 0.9375f, // Bottom-left
         0.5f, -0.5f,  0.5f,  0.0625f, 0.9375f, // Bottom-right
         0.5f,  0.5f,  0.5f,  0.0625f, 1.0f, // Top-right
         0.5f,  0.5f,  0.5f,  0.0625f, 1.0f, // Top-right
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // Top-left
         0.5f, -0.5f, -0.5f,  0.0f, 0.9375f, // Bottom-left

        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, 0.9375f, // Bottom-left
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, // Top-left
         0.5f, -0.5f,  0.5f,  0.0625f, 1.0f, // Top-right
         0.5f, -0.5f,  0.5f,  0.0625f, 1.0f, // Top-right
         0.5f, -0.5f, -0.5f,  0.0625f, 0.9375f, // Bottom-right
        -0.5f, -0.5f, -0.5f,  0.0f, 0.9375f, // Bottom-left

        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f, 0.9375f, // Bottom-left
         0.5f,  0.5f, -0.5f,  0.0625f, 0.9375f, // Bottom-right
         0.5f,  0.5f,  0.5f,  0.0625f, 1.0f, // Top-right
         0.5f,  0.5f,  0.5f,  0.0625f, 1.0f, // Top-right
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // Top-left
        -0.5f,  0.5f, -0.5f,  0.0f, 0.9375f  // Bottom-left
    };




    /*unsigned int indices[] = {
        // Left
        3, 7, 2,
        7, 6, 2,
        // Right
        0, 4, 1,
        1, 4, 5,
        // Front
        0, 1, 3,
        1, 2, 3,
    };*/

    unsigned int VAO1, VBO1;
    glGenVertexArrays(1, &VAO1);
    glGenBuffers(1, &VBO1);

    glBindVertexArray(VAO1);

    glBindBuffer(GL_ARRAY_BUFFER, VBO1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);


    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load("../images/terrain.png", &width, &height, &nrChannels, 0);
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    Shader shaderGay("../shaders/Gay.vert", "../shaders/Gay.frag");

    glEnable(GL_DEPTH_TEST);

    const siv::PerlinNoise::seed_type seed = 123456u;
    const siv::PerlinNoise perlin{ seed };

    int worldSizeX = 128, worldSizeY = 30, worldSizeZ = 128;
    std::vector blocks(worldSizeX, std::vector(worldSizeY, std::vector(worldSizeZ, Blocks::AIR)));

    /*for (int x = 0; x < worldSizeX; x++) {
        for (int y = 0; y < worldSizeY; y++) {
            for (int z = 0; z < worldSizeZ; z++) {
                blocks[x][y][z] = Blocks::DIRT;
                if (y == worldSizeY - 1) blocks[x][y][z] = Blocks::GRASS_BLOCK;
            }
        }
    }*/

    for (int x = 0; x < worldSizeX; x++) {
        for (int z = 0; z < worldSizeZ; z++) {
            const double noise = perlin.octave2D_01((x * 0.01), (z * 0.01), 4);
            const int blockHeight = static_cast<int>(worldSizeY * noise);
            for (int y = 0; y < blockHeight; y++) {
                blocks[x][y][z] = Blocks::DIRT;
                if (y == blockHeight - 1) blocks[x][y][z] = Blocks::GRASS_BLOCK;
            }
        }
    }

    blocks[worldSizeX - 1][worldSizeY - 1][worldSizeZ - 1] = Blocks::AIR;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND );
    glEnable(GL_CULL_FACE); // Enable face culling
    glCullFace(GL_BACK);    // Cull back-facing triangles
    glFrontFace(GL_CW);    // Set counter-clockwise vertices as the front face

    glm::vec4 grassTint = getPixelColor("../images/grasscolor.png", 0, 30);

    std::cout << "R: " << grassTint.r << " G: " << grassTint.g << " B: " << grassTint.b << std::endl;

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scroll_callback);

        glClearColor(0.5294f, 0.8078f, 0.9216f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT), 0.1f, 100.0f);
        glm::mat4 viewProjection = projection * view;

        auto frustumPlanes = extractFrustumPlanes(viewProjection);

        shaderGay.use();
        shaderGay.setMat4("view", view);
        shaderGay.setMat4("projection", projection);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindVertexArray(VAO1);

        shaderGay.setInt("ourTexture", 0);
        shaderGay.setInt("topTexture", 1);

        for (int x = 0; x < worldSizeX; x++) {
            for (int y = 0; y < worldSizeY; y++) {
                for (int z = 0; z < worldSizeZ; z++) {
                    if (blocks[x][y][z] == Blocks::AIR) continue;

                    glm::vec3 min = glm::vec3(x, y, z);
                    glm::vec3 max = glm::vec3(x + 1, y + 1, z + 1);

                    if (!isBoxInFrustum(frustumPlanes, min, max)) continue;

                    auto model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(x, y, z));
                    shaderGay.setMat4("model", model);

                    shaderGay.setVec2("textureOff2", glm::vec2{4, -11} * 0.0625f); // Top face offset
                    // Top face
                    if (y + 1 >= worldSizeY || blocks[x][y + 1][z] == Blocks::AIR) {
                        if (blocks[x][y][z] == Blocks::GRASS_BLOCK) {
                            shaderGay.setVec4("tintColor", grassTint);
                            shaderGay.setVec2("textureOff2", glm::vec2{0, 0} * 0.0625f); // Top face offset
                        }
                        shaderGay.setVec2("textureOff", blocks[x][y][z].textureOffsets[4] * 0.0625f); // Top face offset
                        glDrawArrays(GL_TRIANGLES, 30, 6);
                    }

                    // Bottom face
                    if (y - 1 < 0 || blocks[x][y - 1][z] == Blocks::AIR) {
                        shaderGay.setVec2("textureOff", blocks[x][y][z].textureOffsets[5] * 0.0625f); // Bottom face offset
                        glDrawArrays(GL_TRIANGLES, 24, 6);
                    }

                    if (blocks[x][y][z] == Blocks::GRASS_BLOCK) {
                        shaderGay.setVec2("textureOff2", glm::vec2(6, -2) *  0.0625f);
                    }
                    // Right face
                    if (x + 1 >= worldSizeX || blocks[x + 1][y][z] == Blocks::AIR) {
                        shaderGay.setVec2("textureOff", blocks[x][y][z].textureOffsets[3] * 0.0625f); // Right face offset
                        glDrawArrays(GL_TRIANGLES, 18, 6);
                    }

                    // Left face
                    if (x - 1 < 0 || blocks[x - 1][y][z] == Blocks::AIR) {
                        shaderGay.setVec2("textureOff", blocks[x][y][z].textureOffsets[2] * 0.0625f); // Left face offset
                        glDrawArrays(GL_TRIANGLES, 12, 6);
                    }

                    // Back face
                    if (z + 1 >= worldSizeZ || blocks[x][y][z + 1] == Blocks::AIR) {
                        shaderGay.setVec2("textureOff", blocks[x][y][z].textureOffsets[1] * 0.0625f); // Back face offset
                        glDrawArrays(GL_TRIANGLES, 6, 6);
                    }

                    // Front face
                    if (z - 1 < 0 || blocks[x][y][z - 1] == Blocks::AIR) {
                        shaderGay.setVec2("textureOff", blocks[x][y][z].textureOffsets[0] * 0.0625f); // Front face offset
                        glDrawArrays(GL_TRIANGLES, 0, 6);
                    }
                }
            }
        }

        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO1);
    glDeleteBuffers(1, &VBO1);

    glfwTerminate();
    return 0;
}




