#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "shader.h"
#include "camera.h"
#define STB_IMAGE_IMPLEMENTATION
#include <optional>
#include <thread>
#include "stb_image.h"
#include <unordered_map>
#include "chunk.cpp"

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float yaw, pitch, fov = 90;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

constexpr int BLOCKS_PER_CHUNK = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;
constexpr int blocksInWorld = BLOCKS_PER_CHUNK * chunkSizeX * chunkSizeZ;

std::vector chunks(chunkSizeX, std::vector(chunkSizeZ, Chunk(0, 0)));

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


void mouse_callback(GLFWwindow* window, const double xPosIn, const double yPosIn)
{
    const auto xPos = static_cast<float>(xPosIn);
    const auto yPos = static_cast<float>(yPosIn);

    if (firstMouse)
    {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }

    const float xOffset = xPos - lastX;
    const float yOffset = lastY - yPos; // reversed since y-coordinates go from bottom to top

    lastX = xPos;
    lastY = yPos;

    camera.ProcessMouseMovement(xOffset, yOffset);
}


void scroll_callback(GLFWwindow* window, double xOffset, const double yOffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yOffset));
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

    // Calculate the indexes of the pixel in the data array
    const int index = (y * width + x) * 4; // Multiply by 4 for RGBA

    // Extract the RGBA components
    const unsigned char r = data[index];
    const unsigned char g = data[index + 1];
    const unsigned char b = data[index + 2];
    const unsigned char a = data[index + 3];

    // Free the image memory
    stbi_image_free(data);

    // Convert the components to a normalized float vector (0.0 - 1.0)
    return {static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f, static_cast<float>(a) / 255.0f};
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

    float faceVertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.9375f, // Bottom-left
         0.5f, -0.5f, -0.5f,  0.0625f, 0.9375f, // Bottom-right
         0.5f,  0.5f, -0.5f,  0.0625f, 1.0f, // Top-right
         0.5f,  0.5f, -0.5f,  0.0625f, 1.0f, // Top-right
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // Top-left
        -0.5f, -0.5f, -0.5f,  0.0f, 0.9375f, // Bottom-left
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(faceVertices), faceVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    std::cout << "Starting chunk initialization" << std::endl;
    std::vector<std::thread> threads;
    for (int x = 0; x < chunkSizeX; x++) {
        for (int z = 0; z < chunkSizeZ; z++) {
            chunks[x][z] = Chunk(x, z);
            std::cout << "Initialized Chunk at (" << x << ", " << z << ")\n";
        }
    }

    std::mutex chunkMutex;

    for (int x = 0; x < chunkSizeX; x++) {
        for (int z = 0; z < chunkSizeZ; z++) {
            threads.emplace_back([&chunk = chunks[x][z], x, z, positiveX = (x + 1 < chunkSizeX ? &chunks[x + 1][z] : nullptr),
                                  negativeX = (x - 1 >= 0 ? &chunks[x - 1][z] : nullptr),
                                  positiveZ = (z + 1 < chunkSizeZ ? &chunks[x][z + 1] : nullptr),
                                  negativeZ = (z - 1 >= 0 ? &chunks[x][z - 1] : nullptr), &chunkMutex]() {
                                      std::lock_guard<std::mutex> lock(chunkMutex);
                chunk.generateChunkData(x, z, positiveX, negativeX, positiveZ, negativeZ);
            });
            /*Chunk* nX = nullptr;
            if (x - 1 >= 0) {
                nX = &chunks[x - 1][z];
            }
            Chunk* pX = nullptr;
            if (x + 1 < chunkSizeX) {
                pX = &chunks[x + 1][z];
            }
            Chunk* nZ = nullptr;
            if (z - 1 >= 0) {
                nZ = &chunks[x][z - 1];
            }
            Chunk* pZ = nullptr;
            if (z + 1 < chunkSizeZ) {
                pZ = &chunks[x][z + 1];
            }
            chunks[x][z].generateChunkData(x, z, pX, nX, pZ, nZ);*/
        }
    }

    /*for (auto& thread : threads) {
        thread.join();
    }*/


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


    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);



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

        const auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT), 0.1f, 1000.0f);


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
        shaderGay.setVec4("tintColor", grassTint);

        glBindVertexArray(VAO1);
        for (int x = 0; x < chunkSizeX; x++) {
            for (int z = 0; z < chunkSizeZ; z++) {
                //chunks[x][z].setupBuffer();
                GLuint instanceVBO;
                glGenBuffers(1, &instanceVBO);

                glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
                glBufferData(GL_ARRAY_BUFFER, chunks[x][z].combinedData.size() * sizeof(float), chunks[x][z].combinedData.data(), GL_STATIC_DRAW);
                constexpr GLsizei stride = (2 + 2 + 16) * sizeof(float);
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, static_cast<void *>(nullptr));
                glVertexAttribDivisor(2, 1);

                glEnableVertexAttribArray(3);
                glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(sizeof(float) * 2));
                glVertexAttribDivisor(3, 1);

                glEnableVertexAttribArray(4);
                glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(sizeof(float) * 4));
                glVertexAttribDivisor(4, 1);

                glEnableVertexAttribArray(5);
                glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(sizeof(float) * 8));
                glVertexAttribDivisor(5, 1);

                glEnableVertexAttribArray(6);
                glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(sizeof(float) * 12));
                glVertexAttribDivisor(6, 1);

                glEnableVertexAttribArray(7);
                glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(sizeof(float) * 16));
                glVertexAttribDivisor(7, 1);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glDrawArraysInstanced(GL_TRIANGLES, 0, 6, chunks[x][z].combinedData.size() / 20);
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




