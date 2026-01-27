#include "chunkgeneration.h"

// generate basic chunk with stone from worldCords y = 0 to y = 16
std::unique_ptr<Chunk>
ChunkGenerator::generateChunk(const WorldChunkPos &chunkPos) {

  std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>();
  if (chunkPos.y == 0) {
    for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
      for (int y = 0; y < Chunk::CHUNK_SIZE; y++) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; z++) {
          if (y < 16) {
            chunk->setBlock(x, y, z, Block(1));
          } else {
            chunk->setBlock(x, y, z, Block(0));
          }
        }
      }
    }
  }
  return chunk;
}