#ifndef CHUNKGENERATION_H
#define CHUNKGENERATION_H

#include "region.h"
#include "world.h"
#include <memory>

class ChunkGenerator {
public:
  static std::unique_ptr<Chunk> generateChunk(const RegionPos &regionPos,
                                              const ChunkPos &chunkPos);
};

#endif // CHUNKGENERATION_H