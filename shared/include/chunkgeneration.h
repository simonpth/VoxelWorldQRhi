#ifndef CHUNKGENERATION_H
#define CHUNKGENERATION_H

#include "world.h"
#include <memory>

class ChunkGenerator {
public:
  static std::unique_ptr<Chunk> generateChunk(const WorldChunkPos &chunkPos);
};

#endif // CHUNKGENERATION_H