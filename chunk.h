#ifndef CHUNK_H
#define CHUNK_H

#include <vector>
#include "block.cpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

constexpr int CHUNK_SIZE_X = 16;
constexpr int CHUNK_SIZE_Y = 128;
constexpr int CHUNK_SIZE_Z = 16;

class Chunk {
public:
    std::vector<std::vector<std::vector<Block>>> blocks;
    std::vector<float> combinedData;

    inline Chunk(int chunkX, int chunkZ);

    inline void generateChunk(int chunkX, int chunkZ);
    inline void generateFaces(int x, int z);
};

#endif // CHUNK_H