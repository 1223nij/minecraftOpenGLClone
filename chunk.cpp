#include "chunk.h"
#include "settings.cpp"
#include <iostream>

Chunk::Chunk(int chunkX, int chunkZ) : chunkX(chunkX), chunkZ(chunkZ){
    blocks.resize(
        CHUNK_SIZE_X,
        std::vector(
            CHUNK_SIZE_Y,
            std::vector(CHUNK_SIZE_Z, Blocks::AIR)
        )
    );
    generateChunk(chunkX, chunkZ);
}

bool Chunk::operator==(const Chunk& other) const {
    return this->chunkX == other.chunkX && this->chunkZ == other.chunkZ;
}

void Chunk::generateChunk(int chunkX, int chunkZ) {
    FastNoiseLite baseNoise, detailNoise, biomeNoise, caveNoise, tunnelNoise;

    // Set seeds for reproducibility
    baseNoise.SetSeed(seed);
    detailNoise.SetSeed(seed + 1);
    biomeNoise.SetSeed(seed + 2);
    caveNoise.SetSeed(seed + 3);
    tunnelNoise.SetSeed(seed + 4);

    // Configure noise types and frequencies
    baseNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    baseNoise.SetFrequency(0.01f); // Large-scale terrain

    detailNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    detailNoise.SetFrequency(0.05f); // Small-scale features

    biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    biomeNoise.SetFrequency(0.005f); // Biome distribution

    caveNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    caveNoise.SetFrequency(0.08f); // Higher frequency for smaller caves

    tunnelNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    tunnelNoise.SetFrequency(0.05f); // Frequency for directional tunnels

    const int SEA_LEVEL = CHUNK_SIZE_Y / 8; // Define a water level (quarter of max height)

    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
            // Calculate world coordinates
            double worldX = (chunkX * CHUNK_SIZE_X + x) * 1.0;
            double worldZ = (chunkZ * CHUNK_SIZE_Z + z) * 1.0;

            // Biome noise determines the biome type
            double biomeValue = biomeNoise.GetNoise(worldX, worldZ);
            bool isForest = (biomeValue > 0.2);
            bool isDesert = (biomeValue < -0.2);

            // Base terrain height
            double baseHeight = baseNoise.GetNoise(worldX, worldZ) * 10 + 60;

            // Add detail to terrain
            double detailHeight = detailNoise.GetNoise(worldX, worldZ) * 5;

            // Combine base and detail noise for final terrain height
            int blockHeight = static_cast<int>(baseHeight + detailHeight);

            // Clamp block height to the chunk's maximum height
            blockHeight = std::min(blockHeight, CHUNK_SIZE_Y - 1);

            // Generate terrain layers
            for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
                if (y < blockHeight) {
                    if (y < blockHeight - 4) {
                        blocks[x][y][z] = Blocks::STONE; // Desert biome uses sand
                    } else if (isDesert) {
                        blocks[x][y][z] = Blocks::SAND; // Underground stone layer
                    } else if (y < blockHeight - 1) {
                        blocks[x][y][z] = Blocks::DIRT; // Dirt layer
                    } else {
                        blocks[x][y][z] = Blocks::GRASS_BLOCK; // Top grass layer
                    }
                } else if (y < SEA_LEVEL) {
                    //blocks[x][y][z] = Blocks::WATER; // Fill water below sea level
                }
            }

            // Small cave system generation
            for (int y = 0; y < blockHeight; ++y) {
                // Cave noise for small pockets
                double caveValue = caveNoise.GetNoise(worldX, y * 1.0, worldZ);
                if (caveValue > 0.55) { // Adjust threshold for small caves
                    blocks[x][y][z] = Blocks::AIR; // Carve out a small cave
                }
            }

            // Tunnel generation with directional noise
            for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
                // Create worm-like tunnels with directional bias
                double tunnelValue = tunnelNoise.GetNoise(worldX * 0.5, y * 0.2, worldZ * 0.5);
                if (tunnelValue > 0.65 && tunnelValue < 0.8) { // Narrow range for tunnels
                    blocks[x][y][z] = Blocks::AIR; // Carve tunnel
                }
            }

            // Add trees in forest biomes with some probability
            if (isForest && blockHeight > SEA_LEVEL + 2 && (rand() % 100) < 10) { // 10% chance
                generateTree(x, blockHeight, z);
            }

            // Add cacti in desert biomes with some probability
            if (isDesert && blockHeight > SEA_LEVEL + 2 && (rand() % 100) < 5) { // 5% chance
                generateCactus(x, blockHeight, z);
            }
        }
    }
}

void Chunk::generateCactus(int x, int baseHeight, int z) {
    // Cactus parameters
    const int MAX_CACTUS_HEIGHT = 4; // Maximum cactus height
    const int MIN_CACTUS_HEIGHT = 2; // Minimum cactus height

    // Random height for the cactus
    int cactusHeight = MIN_CACTUS_HEIGHT + (rand() % (MAX_CACTUS_HEIGHT - MIN_CACTUS_HEIGHT + 1));

    // Generate the cactus
    for (int y = 0; y < cactusHeight; ++y) {
        int currentY = baseHeight + y;

        // Ensure the cactus doesn't exceed the chunk's height limit
        if (currentY >= CHUNK_SIZE_Y) {
            break;
        }

        // Place cactus block
        blocks[x][currentY][z] = Blocks::CACTUS;
    }
}


void Chunk::generateTree(int x, int baseHeight, int z) {
    if (baseHeight + 5 >= CHUNK_SIZE_Y) return;
    if (z + 2 >= CHUNK_SIZE_Z) return;
    if (z - 2 < 0) return;
    if (x + 2 >= CHUNK_SIZE_X) return;
    if (x - 2 < 0) return;

    // Layer 1
    blocks[x][baseHeight][z] = Blocks::OAK_LOG;

    // Layer 2
    blocks[x][baseHeight + 1][z] = Blocks::OAK_LOG;

    // Layer 3
    for (int cx = x - 2; cx <= x + 2; ++cx) {
        for (int cz = z - 2; cz <= z + 2; ++cz) {
            blocks[cx][baseHeight + 2][cz] = Blocks::OAK_LEAVES;
        }
    }
    blocks[x][baseHeight + 2][z] = Blocks::OAK_LOG;

    // Layer 4
    for (int cx = x - 2; cx <= x + 2; ++cx) {
        for (int cz = z - 2; cz <= z + 2; ++cz) {
            blocks[cx][baseHeight + 3][cz] = Blocks::OAK_LEAVES;
        }
    }
    blocks[x][baseHeight + 3][z] = Blocks::OAK_LOG;

    // Layer 5
    for (int cx = x - 1; cx <= x + 1; ++cx) {
        for (int cz = z - 1; cz <= z + 1; ++cz) {
            blocks[cx][baseHeight + 4][cz] = Blocks::OAK_LEAVES;
        }
    }
    blocks[x][baseHeight + 4][z] = Blocks::OAK_LOG;

    // Layer 6
    blocks[x][baseHeight + 5][z] = Blocks::OAK_LEAVES;
    blocks[x][baseHeight + 5][z - 1] = Blocks::OAK_LEAVES;
    blocks[x][baseHeight + 5][z + 1] = Blocks::OAK_LEAVES;
    blocks[x - 1][baseHeight + 5][z] = Blocks::OAK_LEAVES;
    blocks[x + 1][baseHeight + 5][z] = Blocks::OAK_LEAVES;
}

void Chunk::generateChunkData(int x, int z, Chunk* positiveX, Chunk* negativeX, Chunk* positiveZ, Chunk* negativeZ) {
    std::cout << "var\n";
    std::vector<glm::vec2> aTexOffsetOverlay;
    std::vector<glm::vec2> aTexOffset;
    std::vector<glm::mat4> models;
    std::vector<bool> visibility;
    std::vector<glm::vec3> normals;
    int size = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * 6;
    aTexOffsetOverlay.resize(size);
    aTexOffset.resize(size);
    models.resize(size);
    visibility.resize(size);
    normals.resize(size);
    std::cout << "data\n";
    for (int cx = 0; cx < CHUNK_SIZE_X; cx++) {
        for (int cy = 0; cy < CHUNK_SIZE_Y; cy++) {
            for (int cz = 0; cz < CHUNK_SIZE_Z; cz++) {
                glm::vec3 blockLocation = glm::vec3(cx + chunkX * CHUNK_SIZE_X, cy, cz + chunkZ * CHUNK_SIZE_Z);
                for (int i = 0; i < 6; i++) {
                    // Calculate the flat index
                    int index = ((cx * CHUNK_SIZE_Y * CHUNK_SIZE_Z) + (cy * CHUNK_SIZE_Z) + cz) * 6 + i;

                    if (blocks[cx][cy][cz] == Blocks::AIR) {
                        continue;
                    }
                    switch (i) {
                        case 0: // Back face
                            if (cz - 1 < 0) {
                                if (negativeZ != nullptr && negativeZ->blocks[cx][cy][CHUNK_SIZE_Z - 1].isTransparent && !(blocks[cx][cy][cz].isLiquid && blocks[cx][cy][cz] == negativeZ->blocks[cx][cy][CHUNK_SIZE_Z - 1])) {
                                    visibility[index] = true;
                                }
                            } else if (blocks[cx][cy][cz - 1].isTransparent && !(blocks[cx][cy][cz].isLiquid && blocks[cx][cy][cz] == blocks[cx][cy][cz - 1])) {
                                visibility[index] = true;
                            }
                            break;

                        case 1: // Front face
                            if (cz + 1 >= CHUNK_SIZE_Z) {
                                if (positiveZ != nullptr && positiveZ->blocks[cx][cy][0].isTransparent && !(blocks[cx][cy][cz].isLiquid && blocks[cx][cy][cz] == positiveZ->blocks[cx][cy][0])) {
                                    visibility[index] = true;
                                }
                            } else if (blocks[cx][cy][cz + 1].isTransparent && !(blocks[cx][cy][cz].isLiquid && blocks[cx][cy][cz] == blocks[cx][cy][cz + 1])) {
                                visibility[index] = true;
                            }
                            break;

                        case 2: // Left face
                            if (cx - 1 < 0) {
                                if (negativeX != nullptr && negativeX->blocks[CHUNK_SIZE_X - 1][cy][cz].isTransparent && !(blocks[cx][cy][cz].isLiquid && blocks[cx][cy][cz] == negativeX->blocks[CHUNK_SIZE_X - 1][cy][cz])) {
                                    visibility[index] = true;
                                }
                            } else if (blocks[cx - 1][cy][cz].isTransparent && !(blocks[cx][cy][cz].isLiquid && blocks[cx][cy][cz] == blocks[cx - 1][cy][cz])) {
                                visibility[index] = true;
                            }
                            break;

                        case 3: // Right face
                            if (cx + 1 >= CHUNK_SIZE_X) {
                                if (positiveX != nullptr && positiveX->blocks[0][cy][cz].isTransparent && !(blocks[cx][cy][cz].isLiquid && blocks[cx][cy][cz] == positiveX->blocks[0][cy][cz])) {
                                    visibility[index] = true;
                                }
                            } else if (blocks[cx + 1][cy][cz].isTransparent && !(blocks[cx][cy][cz].isLiquid && blocks[cx][cy][cz] == blocks[cx + 1][cy][cz])) {
                                visibility[index] = true;
                            }
                            break;

                        case 4: // Top face
                            if (cy + 1 >= CHUNK_SIZE_Y || blocks[cx][cy + 1][cz].isTransparent && !(blocks[cx][cy][cz].isLiquid && blocks[cx][cy][cz] == blocks[cx][cy + 1][cz])) {
                                visibility[index] = true;
                            }
                            break;

                        case 5: // Bottom face
                            if (cy - 1 < 0 || blocks[cx][cy - 1][cz].isTransparent && !(blocks[cx][cy][cz].isLiquid && blocks[cx][cy][cz] == blocks[cx][cy - 1][cz])) {
                                visibility[index] = true;
                            }
                            break;
                        default: ;
                    }

                    if (visibility[index] == false) {
                        continue;
                    }

                    // Assign texture offsets
                    aTexOffset[index] = blocks[cx][cy][cz].textureOffsets[i];
                    aTexOffsetOverlay[index] = blocks[cx][cy][cz].textureOffsetOverlays[i];

                    // Calculate the model matrix
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), blockLocation);
                    if (blocks[cx][cy][cz] == Blocks::CACTUS) {
                        switch (i) {
                            case 0:
                                model = glm::translate(glm::mat4(1.0f), glm::vec3(blockLocation.x, blockLocation.y, blockLocation.z + 1.0f / 16));
                                break;
                            case 1:
                                model = glm::translate(glm::mat4(1.0f), glm::vec3(blockLocation.x, blockLocation.y, blockLocation.z - 1.0f / 16));
                                break;
                            case 2:
                                model = glm::translate(glm::mat4(1.0f), glm::vec3(blockLocation.x + 1.0f / 16, blockLocation.y, blockLocation.z));
                                break;
                            case 3:
                                model = glm::translate(glm::mat4(1.0f), glm::vec3(blockLocation.x - 1.0f / 16, blockLocation.y, blockLocation.z));
                                break;
                            default: ;
                        }
                    }
                    switch (i) {
                        case 0:
                            normals[index] = glm::vec3(0.0f,  0.0f, -1.0f);
                            break; // Front face
                        case 1:
                            normals[index] = glm::vec3(0.0f,  0.0f, 1.0f);
                            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                            break; // Back face
                        case 2:
                            normals[index] = glm::vec3(-1.0f,  0.0f, 0.0f);
                            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                            break; // Left face
                        case 3:
                            normals[index] = glm::vec3(1.0f,  0.0f, 0.0f);
                            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                            break; // Right face
                        case 4:
                            normals[index] = glm::vec3(0.0f,  1.0f, 0.0f);
                            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                            break; // Bottom face
                        case 5:
                            normals[index] = glm::vec3(0.0f,  -1.0f, 0.0f);
                            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                            break; // Top face
                        default: ;
                    }

                    // Assign the model matrix
                    models[index] = model;

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
            normals[writeIndex] = normals[readIndex];
            ++writeIndex;
        }
    }
    aTexOffset.resize(writeIndex);
    aTexOffsetOverlay.resize(writeIndex);
    models.resize(writeIndex);
    visibility.resize(writeIndex);
    normals.resize(writeIndex);


    if (models.size() == visibility.size() && visibility.size() == aTexOffset.size() && aTexOffset.size() == aTexOffsetOverlay.size()) {
        std::cout << "All vectors are the same size: " << models.size() << std::endl;
        size = static_cast<int>(models.size());
    } else {
        std::cout << "Vectors are not the same size" << std::endl;
    }
    std::cout << "hallo" << std::endl;
    combinedData = std::vector<float>(size * (2 + 2 + 16 + 3)); // 2 floats for each vec2, 16 floats for mat4
    std::cout << "comb\n";
    for (size_t i = 0; i < size; ++i) {
        // Add aTexOffset
        combinedData[i * 23 + 0] = aTexOffset[i].x;
        combinedData[i * 23 + 1] = aTexOffset[i].y;

        // Add aTexOffsetOverlay
        combinedData[i * 23 + 2] = aTexOffsetOverlay[i].x;
        combinedData[i * 23 + 3] = aTexOffsetOverlay[i].y;

        // Add model matrix
        const glm::mat4& mat = models[i];

        for (int j = 0; j < 4; ++j) {
            combinedData[i * 23 + 4 + 4 * j + 0] = mat[j].x;
            combinedData[i * 23 + 4 + 4 * j + 1] = mat[j].y;
            combinedData[i * 23 + 4 + 4 * j + 2] = mat[j].z;
            combinedData[i * 23 + 4 + 4 * j + 3] = mat[j].w;
        }
        combinedData[i * 23 + 20] = normals[i].x;
        combinedData[i * 23 + 21] = normals[i].y;
        combinedData[i * 23 + 22] = normals[i].z;
    }
}
