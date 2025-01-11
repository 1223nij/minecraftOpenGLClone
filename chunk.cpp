#include "chunk.h"
#include "settings.cpp"

Chunk::Chunk(int chunkX, int chunkZ) : blocks(CHUNK_SIZE_X, std::vector(CHUNK_SIZE_Y, std::vector(CHUNK_SIZE_Z, Blocks::AIR))) {
    blocks.resize(
        CHUNK_SIZE_X,
        std::vector<std::vector<Block>>(
            CHUNK_SIZE_Y,
            std::vector<Block>(CHUNK_SIZE_Z, Blocks::AIR)
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