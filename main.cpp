#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "shader.h"
#include "camera.h"
#define STB_IMAGE_IMPLEMENTATION
#include <map>
#include <optional>
#include <thread>
#include "stb_image.h"
#include <unordered_map>
#include "chunk.cpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float yaw, pitch, fov = 90;

Camera camera(glm::vec3(0.0f, 80.0f, 0.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

constexpr int BLOCKS_PER_CHUNK = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

//std::vector chunks(chunkSizeX, std::vector(chunkSizeZ, Chunk(0, 0)));

std::map<std::pair<int, int>, Chunk> chunkMap;

std::vector<std::thread> threads;
std::vector<Chunk*> renderedChunks;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.ProcessKeyboard(UP, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.ProcessKeyboard(DOWN, deltaTime);
    }
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

std::mutex chunkMutex;  // To protect shared resources
std::atomic<bool> cleanupInProgress(false);  // To prevent overlapping cleanups

void cleanupChunksAsync(const glm::vec2& chunkPosition,
                        std::map<std::pair<int, int>, Chunk>& chunkMap,
                        std::vector<Chunk*>& renderedChunks,
                        int cleanupRadius) {
    if (cleanupInProgress) return; // Skip if a cleanup is already in progress
    cleanupInProgress = true;

    std::thread([chunkPosition, &chunkMap, &renderedChunks, cleanupRadius]() {
        std::vector<std::pair<int, int>> chunksToRemove;

        // Identify chunks to be removed
        {
            std::lock_guard<std::mutex> lock(chunkMutex);
            for (const auto& [position, chunk] : chunkMap) {
                int dx = position.first - static_cast<int>(chunkPosition.x);
                int dz = position.second - static_cast<int>(chunkPosition.y);

                if (std::abs(dx) > cleanupRadius || std::abs(dz) > cleanupRadius) {
                    chunksToRemove.push_back(position);
                }
            }
        }

        // Remove chunks from chunkMap and renderedChunks
        {
            std::lock_guard<std::mutex> lock(chunkMutex);
            for (const auto& position : chunksToRemove) {
                auto it = chunkMap.find(position);
                if (it != chunkMap.end()) {
                    // Remove from chunkMap
                    Chunk* chunkPtr = &it->second;
                    chunkMap.erase(it);

                    // Remove from renderedChunks
                    auto renderedIt = std::ranges::find(renderedChunks, chunkPtr);
                    if (renderedIt != renderedChunks.end()) {
                        renderedChunks.erase(renderedIt);
                    }
                }
            }
        }

        //std::cout << "Async cleanup complete. Removed " << chunksToRemove.size() << " chunks.\n";
        cleanupInProgress = false;
    }).detach(); // Detach the thread to allow it to run independently
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

    /*std::cout << "Starting chunk initialization" << std::endl;
    for (int x = 0; x < chunkSizeX; x++) {
        for (int z = 0; z < chunkSizeZ; z++) {
            chunks[x][z] = Chunk(x, z);
            std::cout << "Initialized Chunk at (" << x << ", " << z << ")\n";
        }
    }*/

    std::mutex chunkMutex;

    /*for (int x = 0; x < chunkSizeX; x++) {
        for (int z = 0; z < chunkSizeZ; z++) {
            threads.emplace_back([&chunk = chunks[x][z], x, z, positiveX = (x + 1 < chunkSizeX ? &chunks[x + 1][z] : nullptr),
                                  negativeX = (x - 1 >= 0 ? &chunks[x - 1][z] : nullptr),
                                  positiveZ = (z + 1 < chunkSizeZ ? &chunks[x][z + 1] : nullptr),
                                  negativeZ = (z - 1 >= 0 ? &chunks[x][z - 1] : nullptr), &chunkMutex]() {
                                      std::lock_guard<std::mutex> lock(chunkMutex);
                chunk.generateChunkData(x, z, positiveX, negativeX, positiveZ, negativeZ);
            });
            Chunk* nX = nullptr;
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
            chunks[x][z].generateChunkData(x, z, pX, nX, pZ, nZ);
        }
    }*/

    /*for (auto& thread : threads) {
        thread.join();
    }*/


    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load("../images/terrain.png", &width, &height, &nrChannels, STBI_rgb_alpha);
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    Shader shaderGay("../shaders/Gay.vert", "../shaders/Gay.frag");
    Shader shaderLight("../shaders/Light.vert", "../shaders/Light.frag");

    glEnable(GL_DEPTH_TEST);


    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);



    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND );
    glEnable(GL_CULL_FACE); // Enable face culling
    glCullFace(GL_BACK);    // Cull back-facing triangles
    glFrontFace(GL_CW);    // Set counter-clockwise vertices as the front face


    glm::vec4 grassTint = getPixelColor("../images/grasscolor.png", 127, 127);

    std::cout << "R: " << grassTint.r << " G: " << grassTint.g << " B: " << grassTint.b << std::endl;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiWindowFlags_AlwaysAutoResize;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    float vertices[] = {
        // Front face
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        // Back face
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        // Left face
        -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        // Right face
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,

        // Bottom face
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        // Top face
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f
    };

    unsigned int lightCubeVAO, lightCubeVBO;
    glGenVertexArrays(1, &lightCubeVAO);
    glGenBuffers(1, &lightCubeVBO);
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(window)) {
        glm::vec2 chunkPosition = glm::vec2(floor(camera.Position.x / 16), floor(camera.Position.z / 16));
        cleanupChunksAsync(chunkPosition, chunkMap, renderedChunks, cleanupRadius);
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (ImGui::Begin("Debugging Information", nullptr, io.ConfigFlags)) {
            ImGui::Text(("Position: " + std::to_string(camera.Position.x) + " " + std::to_string(camera.Position.y) + " " + std::to_string(camera.Position.z)).c_str());
            ImGui::Spacing();
            ImGui::Text(("Block: " + std::to_string(static_cast<int>(floor(camera.Position.x))) + " " +
                std::to_string(static_cast<int>(floor(camera.Position.y))) + " " +
                std::to_string(static_cast<int>(floor(camera.Position.z)))).c_str());
            ImGui::Spacing();
            ImGui::Text(("Chunk: " + std::to_string(static_cast<int>(floor(camera.Position.x / 16))) + " " +
                std::to_string(static_cast<int>(floor(camera.Position.z / 16)))).c_str());
        }
        ImGui::End();
        //ImGui::ShowDemoWindow();
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
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, camera.Position);;

        shaderLight.use();
        glBindVertexArray(lightCubeVAO);
        shaderLight.setMat4("view", view);
        shaderLight.setMat4("projection", projection);
        shaderLight.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        shaderGay.use();
        shaderGay.setMat4("view", view);
        shaderGay.setMat4("projection", projection);
        shaderGay.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        shaderGay.setVec3("lightPos", camera.Position);

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

        GLuint instanceVBOla;
        glGenBuffers(1, &instanceVBOla);
        std::vector<float> combinedDeta;
        int chunkRendered = 0, chunkLooped = 0;
        for (int x = chunkPosition.x - renderDistance; x <= chunkPosition.x + renderDistance; x++) {
            for (int z = chunkPosition.y - renderDistance; z <= chunkPosition.y + renderDistance; z++) {
                chunkLooped++;
                if (!chunkMap.contains({x, z})) {
                    chunkMap.emplace(std::make_pair(x, z), Chunk(x, z));
                    continue;
                }
                Chunk* chunka;
                {
                    std::lock_guard<std::mutex> lock(chunkMutex);
                    if (!chunkMap.contains({x, z})) {
                        chunkMap.emplace(std::make_pair(x, z), Chunk(x, z));
                    }
                    chunka = &chunkMap.at({x, z});
                }
                if (std::ranges::find(renderedChunks, chunka) == renderedChunks.end()) {
                    renderedChunks.push_back(chunka);
                    Chunk* nX = nullptr;
                    if (!chunkMap.contains({x - 1, z})) {
                        chunkMap.emplace(std::make_pair(x - 1, z), Chunk(x - 1, z));
                    }
                    nX = &chunkMap.at({x - 1, z});
                    Chunk* pX = nullptr;
                    if (!chunkMap.contains({x + 1, z})) {
                        chunkMap.emplace(std::make_pair(x + 1, z), Chunk(x + 1, z));
                    }
                    pX = &chunkMap.at({x + 1, z});
                    Chunk* nZ = nullptr;
                    if (!chunkMap.contains({x, z - 1})) {
                        chunkMap.emplace(std::make_pair(x, z - 1), Chunk(x, z - 1));
                    }
                    nZ = &chunkMap.at({x, z - 1});
                    Chunk* pZ = nullptr;
                    if (!chunkMap.contains({x, z + 1})) {
                        chunkMap.emplace(std::make_pair(x, z + 1), Chunk(x, z + 1));
                    }
                    pZ = &chunkMap.at({x, z + 1});
                    //chunka->generateChunkData(x, z, pX, nX, pZ, nZ);
                    threads.emplace_back([chunka, x, z, nX, pX, nZ, pZ]() {
                        chunka->generateChunkData(x, z, pX, nX, pZ, nZ);
                    });
                    continue;
                }
                combinedDeta.insert(std::end(combinedDeta), std::begin(chunka->combinedData), std::end(chunka->combinedData));
                chunkRendered++;
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, instanceVBOla);
        glBufferData(GL_ARRAY_BUFFER, combinedDeta.size() * sizeof(float), combinedDeta.data(), GL_STATIC_DRAW);
        constexpr GLsizei stride = (2 + 2 + 16 + 3) * sizeof(float);
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

        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(sizeof(float) * 20));
        glVertexAttribDivisor(8, 1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, combinedDeta.size() / 21);
        glDeleteBuffers(1, &instanceVBOla);
        glBindVertexArray(0);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO1);
    glDeleteBuffers(1, &VBO1);


    // Cleanup
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    glfwTerminate();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    return 0;
}




