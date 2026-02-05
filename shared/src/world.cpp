#include "world.h"
#include "chunkgeneration.h"
#include "region.h"
#include <cstdint>

World::World() {}

World::~World() {}

const Region *World::getOrGenerateRegion(const RegionPos &pos) {
  auto it = m_regions.find(pos);
  if (it == m_regions.end()) {
    generateRegion(pos);
    it = m_regions.find(pos);
  }
  return it->second.get();
}

const Region *World::generateRegion(const RegionPos &pos) {
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
  return m_regions[pos].get();
}

void World::basicSetup() {
  for (int x = 0; x < 1; x++) {
    for (int y = 0; y < 1; y++) {
      for (int z = 0; z < 1; z++) {
        generateRegion({x, y, z});
      }
    }
  }
}
