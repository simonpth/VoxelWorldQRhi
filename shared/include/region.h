#ifndef REGION_H
#define REGION_H

#include "chunk.h"
#include <array>
#include <cstdint>
#include <memory>

struct ChunkPos {
  uint8_t x, y, z;

  bool operator==(const ChunkPos &other) const {
    return x == other.x && y == other.y && z == other.z;
  }

  ChunkPos(uint8_t x, uint8_t y, uint8_t z) : x(x), y(y), z(z) {}
};

class Region {
public:
  static constexpr int REGION_SIZE = 4;
  static constexpr int REGION_VOLUME = REGION_SIZE * REGION_SIZE * REGION_SIZE;

  const Chunk *chunk(uint8_t x, uint8_t y, uint8_t z) const {
    return m_chunks[localIndex(x, y, z)].get();
  }
  void setChunk(uint8_t x, uint8_t y, uint8_t z, std::unique_ptr<Chunk> chunk) {
    m_chunks[localIndex(x, y, z)] = std::move(chunk);
  }

  // TODO: SVO generation for LOD

  Region();
  ~Region();

private:
  int localIndex(uint8_t x, uint8_t y, uint8_t z) const {
    return z + y * REGION_SIZE + x * REGION_SIZE * REGION_SIZE;
  }
  std::array<std::unique_ptr<Chunk>, REGION_VOLUME> m_chunks;
};

#endif // REGION_H