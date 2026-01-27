#include "world.h"
#include "chunkgeneration.h"
#include <cstdint>

World::World() {}

World::~World() {}

void World::generateRegion(const RegionPos &pos) {
  std::unique_ptr<Region> region = std::make_unique<Region>();
  for (uint8_t x = 0; x < Region::REGION_SIZE; x++) {
    for (uint8_t y = 0; y < Region::REGION_SIZE; y++) {
      for (uint8_t z = 0; z < Region::REGION_SIZE; z++) {
        region->setChunk(x, y, z,
                         ChunkGenerator::generateChunk({pos, {x, y, z}}));
      }
    }
  }

  m_regions[pos] = std::move(region);
}
