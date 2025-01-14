#ifndef CHUNK_H
#define CHUNK_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "block.cpp"
#include <optional>

constexpr int CHUNK_SIZE_X = 16;
constexpr int CHUNK_SIZE_Y = 128;
constexpr int CHUNK_SIZE_Z = 16;

class Chunk {
public:
    std::vector<float> combinedData;

    inline Chunk(int chunkX, int chunkZ);

    inline bool operator==(const Chunk& other) const;

    inline void generateChunk(int chunkX, int chunkZ);
    inline void setupBuffer();
    inline void generateChunkData(int x, int z, Chunk* positiveX, Chunk* negativeX, Chunk* positiveZ, Chunk* negativeZ);
    inline void generateTree(int x, int baseHeight, int z);
private:
    int chunkX; // X coordinate of the chunk
    int chunkZ; // Z coordinate of the chunk
    std::vector<std::vector<std::vector<Block>>> blocks;
};

#endif // CHUNK_H