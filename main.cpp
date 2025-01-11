#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shader.h"
#include "camera.h"
#define STB_IMAGE_IMPLEMENTATION
#include <thread>

#include "stb_image.h"
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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

int instanceSize = blocksInWorld * 6;
std::vector chunks(chunkSizeX, std::vector(chunkSizeZ, Chunk(0, 0)));
std::vector<glm::vec2> aTexOffsetOverlay;
std::vector<glm::vec2> aTexOffset;
std::vector<glm::mat4> models;
std::vector<bool> visibility;

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

void deleteFace(int cn)
{
    visibility.erase(visibility.begin() + cn);
    aTexOffset.erase(aTexOffset.begin() + cn);
    aTexOffsetOverlay.erase(aTexOffsetOverlay.begin() + cn);
    models.erase(models.begin() + cn);
}

void generateChunkFaces(Chunk newChunk, int x, int z) {
     for (int cx = 0; cx < CHUNK_SIZE_X; cx++) {
        for (int cy = 0; cy < CHUNK_SIZE_Y; cy++) {
            for (int cz = 0; cz < CHUNK_SIZE_Z; cz++) {
                glm::vec3 blockLocation = glm::vec3(cx + x * CHUNK_SIZE_X, cy, cz + z * CHUNK_SIZE_Z);
                for (int i = 0; i < 6; i++) {
                    // Calculate the flat index
                    int index = (((x * chunkSizeZ + z) * CHUNK_SIZE_X + cx) * CHUNK_SIZE_Y + cy) * CHUNK_SIZE_Z * 6 + cz * 6 + i;

                    // Assign texture offsets
                    aTexOffset[index] = newChunk.blocks[cx][cy][cz].textureOffsets[i];
                    aTexOffsetOverlay[index] = newChunk.blocks[cx][cy][cz].textureOffsetOverlays[i];

                    // Calculate the model matrix
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), blockLocation);
                    switch (i) {
                        case 0: break; // Front face
                        case 1: model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); break; // Back face
                        case 3: model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); break; // Left face
                        case 2: model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); break; // Right face
                        case 5: model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); break; // Bottom face
                        case 4: model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); break; // Top face
                        default: ;
                    }

                    // Assign the model matrix
                    models[index] = model;

                    // Initialize visibility
                    visibility[index] = false;
                }
            }
        }
    }
}

void generateNewChunkFaces(int x, int z) {
    const Chunk newChunk(x, z);
    generateChunkFaces(newChunk, x, z);
    chunks[x][z] = newChunk;
}

int main()
{
    aTexOffsetOverlay.resize(instanceSize);
    aTexOffset.resize(instanceSize);
    models.resize(instanceSize);
    visibility.resize(instanceSize);
    std::vector<std::thread> threads;
    for (int x = 0; x < chunkSizeX; x++) {
        for (int z = 0; z < chunkSizeZ; z++) {
            threads.emplace_back(generateNewChunkFaces, x, z);
        }
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "Starting visibility check with size: " << visibility.size() << std::endl;

    for (int x = 0; x < chunkSizeX; x++) {
        for (int z = 0; z < chunkSizeZ; z++) {
            Chunk chunk = chunks[x][z];
            for (int cx = 0; cx < CHUNK_SIZE_X; cx++) {
                for (int cy = 0; cy < CHUNK_SIZE_Y; cy++) {
                    for (int cz = 0; cz < CHUNK_SIZE_Z; cz++) {
                        // Skip AIR blocks
                        if (chunk.blocks[cx][cy][cz] == Blocks::AIR) {
                            continue;
                        }

                        for (int i = 0; i < 6; i++) {
                            int index = ((((x * chunkSizeZ + z) * CHUNK_SIZE_X + cx) * CHUNK_SIZE_Y + cy) * CHUNK_SIZE_Z + cz) * 6 + i;

                            switch (i) {
                                case 0: // Back face
                                    if (cz - 1 < 0) {
                                        if (z - 1 >= 0 && chunks[x][z - 1].blocks[cx][cy][CHUNK_SIZE_Z - 1] == Blocks::AIR) {
                                            visibility[index] = true;
                                        }
                                    } else if (chunk.blocks[cx][cy][cz - 1] == Blocks::AIR) {
                                        visibility[index] = true;
                                    }
                                    break;

                                case 1: // Front face
                                    if (cz + 1 >= CHUNK_SIZE_Z) {
                                        if (z + 1 < chunks[x].size() && chunks[x][z + 1].blocks[cx][cy][0] == Blocks::AIR) {
                                            visibility[index] = true;
                                        }
                                    } else if (chunk.blocks[cx][cy][cz + 1] == Blocks::AIR) {
                                        visibility[index] = true;
                                    }
                                    break;

                                case 2: // Left face
                                    if (cx - 1 < 0) {
                                        if (x - 1 >= 0 && chunks[x - 1][z].blocks[CHUNK_SIZE_X - 1][cy][cz] == Blocks::AIR) {
                                            visibility[index] = true;
                                        }
                                    } else if (chunk.blocks[cx - 1][cy][cz] == Blocks::AIR) {
                                        visibility[index] = true;
                                    }
                                    break;

                                case 3: // Right face
                                    if (cx + 1 >= CHUNK_SIZE_X) {
                                        if (x + 1 < chunks.size() && chunks[x + 1][z].blocks[0][cy][cz] == Blocks::AIR) {
                                            visibility[index] = true;
                                        }
                                    } else if (chunk.blocks[cx + 1][cy][cz] == Blocks::AIR) {
                                        visibility[index] = true;
                                    }
                                    break;

                                case 4: // Top face
                                    if (cy + 1 >= CHUNK_SIZE_Y || chunk.blocks[cx][cy + 1][cz] == Blocks::AIR) {
                                        visibility[index] = true;
                                    }
                                    break;

                                case 5: // Bottom face
                                    if (cy - 1 < 0 || chunk.blocks[cx][cy - 1][cz] == Blocks::AIR) {
                                        visibility[index] = true;
                                    }
                                    break;
                                default: ;
                            }
                        }
                    }
                }
            }
        }
    }


    std::cout << "Visibility check complete" << std::endl;

    // Filter out invalid elements from all related vectors based on `visibility`.
    size_t writeIndex = 0; // Tracks the index for writing valid elements
    for (size_t readIndex = 0; readIndex < visibility.size(); ++readIndex) {
        if (visibility[readIndex]) {
            // Keep valid elements
            aTexOffset[writeIndex] = aTexOffset[readIndex];
            aTexOffsetOverlay[writeIndex] = aTexOffsetOverlay[readIndex];
            models[writeIndex] = models[readIndex];
            visibility[writeIndex] = visibility[readIndex];
            ++writeIndex;
        }
    }

    // Resize vectors to remove extra elements
    aTexOffset.resize(writeIndex);
    aTexOffsetOverlay.resize(writeIndex);
    models.resize(writeIndex);
    visibility.resize(writeIndex);


    if (models.size() == visibility.size() && visibility.size() == aTexOffset.size() && aTexOffset.size() == aTexOffsetOverlay.size()) {
        std::cout << "All vectors are the same size: " << models.size() << std::endl;
        instanceSize = static_cast<int>(models.size());
    } else {
        std::cout << "Vectors are not the same size" << std::endl;
    }

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


    std::vector<float> combinedData(instanceSize * (2 + 2 + 16)); // 2 floats for each vec2, 16 floats for mat4

    for (size_t i = 0; i < instanceSize; ++i) {
        // Add aTexOffset
        combinedData[i * 20 + 0] = aTexOffset[i].x;
        combinedData[i * 20 + 1] = aTexOffset[i].y;

        // Add aTexOffsetOverlay
        combinedData[i * 20 + 2] = aTexOffsetOverlay[i].x;
        combinedData[i * 20 + 3] = aTexOffsetOverlay[i].y;

        // Add model matrix
        const glm::mat4& mat = models[i];

        for (int j = 0; j < 4; ++j) {
            combinedData[i * 20 + 4 + 4 * j + 0] = mat[j].x;
            combinedData[i * 20 + 4 + 4 * j + 1] = mat[j].y;
            combinedData[i * 20 + 4 + 4 * j + 2] = mat[j].z;
            combinedData[i * 20 + 4 + 4 * j + 3] = mat[j].w;
        }
    }

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    unsigned int instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindVertexArray(VAO1);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    glBufferData(GL_ARRAY_BUFFER, combinedData.size() * sizeof(float), combinedData.data(), GL_STATIC_DRAW);


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

    glBindVertexArray(0);

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
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT), 0.1f, 100.0f);


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

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instanceSize);

        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO1);
    glDeleteBuffers(1, &VBO1);
    glDeleteBuffers(1, &instanceVBO);

    glfwTerminate();
    return 0;
}




