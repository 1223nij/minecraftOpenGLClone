#ifndef CHUNK_H
#define CHUNK_H

#include <vector>
#include "block.cpp"

constexpr int CHUNK_SIZE_X = 16;
constexpr int CHUNK_SIZE_Y = 128;
constexpr int CHUNK_SIZE_Z = 16;

class Chunk {
public:
    std::vector<std::vector<std::vector<Block>>> blocks;

    inline Chunk(int chunkX, int chunkZ);

    inline void generateChunk(int chunkX, int chunkZ);
};

#endif // CHUNK_H