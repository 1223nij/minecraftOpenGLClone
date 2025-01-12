#include "chunk.h"
#include "settings.cpp"
#include <iostream>

Chunk::Chunk(int chunkX, int chunkZ){
    blocks.resize(
        CHUNK_SIZE_X,
        std::vector(
            CHUNK_SIZE_Y,
            std::vector(CHUNK_SIZE_Z, Blocks::AIR)
        )
    );
    generateChunk(chunkX, chunkZ);
}

void Chunk::generateChunk(int chunkX, int chunkZ) {
    const siv::PerlinNoise perlin{ seed };
    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
            double noise = perlin.octave2D_01((chunkX * CHUNK_SIZE_X + x) * 0.01, (chunkZ * CHUNK_SIZE_Z + z) * 0.01, 4);
            const int blockHeight = static_cast<int>(CHUNK_SIZE_Y * noise);

            for (int y = 0; y < blockHeight; ++y) {
                blocks[x][y][z] = Blocks::DIRT;
                if (y == blockHeight - 1) {
                    blocks[x][y][z] = Blocks::GRASS_BLOCK;
                }
            }
        }
    }
}

void Chunk::generateChunkData(int x, int z, Chunk* positiveX, Chunk* negativeX, Chunk* positiveZ, Chunk* negativeZ) {
    std::cout << "var\n";
    std::vector<glm::vec2> aTexOffsetOverlay;
    std::vector<glm::vec2> aTexOffset;
    std::vector<glm::mat4> models;
    std::vector<bool> visibility;
    int size = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * 6;
    aTexOffsetOverlay.resize(size);
    aTexOffset.resize(size);
    models.resize(size);
    visibility.resize(size);
    std::cout << "data\n";
    for (int cx = 0; cx < CHUNK_SIZE_X; cx++) {
        for (int cy = 0; cy < CHUNK_SIZE_Y; cy++) {
            for (int cz = 0; cz < CHUNK_SIZE_Z; cz++) {
                glm::vec3 blockLocation = glm::vec3(cx + x * CHUNK_SIZE_X, cy, cz + z * CHUNK_SIZE_Z);
                for (int i = 0; i < 6; i++) {
                    // Calculate the flat index
                    int index = ((cx * CHUNK_SIZE_Y * CHUNK_SIZE_Z) + (cy * CHUNK_SIZE_Z) + cz) * 6 + i;

                    // Assign texture offsets
                    aTexOffset[index] = blocks[cx][cy][cz].textureOffsets[i];
                    aTexOffsetOverlay[index] = blocks[cx][cy][cz].textureOffsetOverlays[i];

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
    std::cout << "face\n";
    for (int cx = 0; cx < CHUNK_SIZE_X; cx++) {
        for (int cy = 0; cy < CHUNK_SIZE_Y; cy++) {
            for (int cz = 0; cz < CHUNK_SIZE_Z; cz++) {
                // Skip AIR blocks
                if (blocks[cx][cy][cz] == Blocks::AIR) {
                    continue;
                }

                for (int i = 0; i < 6; i++) {
                    int index = ((cx * CHUNK_SIZE_Y * CHUNK_SIZE_Z) + (cy * CHUNK_SIZE_Z) + cz) * 6 + i;

                    switch (i) {
                        case 0: // Back face
                            if (cz - 1 < 0) {
                                if (negativeZ != nullptr && negativeZ->blocks[cx][cy][CHUNK_SIZE_Z - 1] == Blocks::AIR) {
                                    visibility[index] = true;
                                }
                            } else if (blocks[cx][cy][cz - 1] == Blocks::AIR) {
                                visibility[index] = true;
                            }
                            break;

                        case 1: // Front face
                            if (cz + 1 >= CHUNK_SIZE_Z) {
                                if (positiveZ != nullptr && positiveZ->blocks[cx][cy][0] == Blocks::AIR) {
                                    visibility[index] = true;
                                }
                            } else if (blocks[cx][cy][cz + 1] == Blocks::AIR) {
                                visibility[index] = true;
                            }
                            break;

                        case 2: // Left face
                            if (cx - 1 < 0) {
                                if (negativeX != nullptr && negativeX->blocks[CHUNK_SIZE_X - 1][cy][cz] == Blocks::AIR) {
                                    visibility[index] = true;
                                }
                            } else if (blocks[cx - 1][cy][cz] == Blocks::AIR) {
                                visibility[index] = true;
                            }
                            break;

                        case 3: // Right face
                            if (cx + 1 >= CHUNK_SIZE_X) {
                                if (positiveX != nullptr && positiveX->blocks[0][cy][cz] == Blocks::AIR) {
                                    visibility[index] = true;
                                }
                            } else if (blocks[cx + 1][cy][cz] == Blocks::AIR) {
                                visibility[index] = true;
                            }
                            break;

                        case 4: // Top face
                            if (cy + 1 >= CHUNK_SIZE_Y || blocks[cx][cy + 1][cz] == Blocks::AIR) {
                                visibility[index] = true;
                            }
                            break;

                        case 5: // Bottom face
                            if (cy - 1 < 0 || blocks[cx][cy - 1][cz] == Blocks::AIR) {
                                visibility[index] = true;
                            }
                            break;
                        default: ;
                    }
                }
            }
        }
    }
    std::cout << "size before: " << models.size() << std::endl;
    std::cout << "vis\n";
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
    aTexOffset.resize(writeIndex);
    aTexOffsetOverlay.resize(writeIndex);
    models.resize(writeIndex);
    visibility.resize(writeIndex);


    if (models.size() == visibility.size() && visibility.size() == aTexOffset.size() && aTexOffset.size() == aTexOffsetOverlay.size()) {
        std::cout << "All vectors are the same size: " << models.size() << std::endl;
        size = static_cast<int>(models.size());
    } else {
        std::cout << "Vectors are not the same size" << std::endl;
    }
    std::cout << "hallo" << std::endl;
    combinedData = std::vector<float>(size * (2 + 2 + 16)); // 2 floats for each vec2, 16 floats for mat4
    std::cout << "comb\n";
    for (size_t i = 0; i < size; ++i) {
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
}
