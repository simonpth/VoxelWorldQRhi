#ifndef REGION_H
#define REGION_H

#include "chunk.h"
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>

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

  /**
   * @brief Claims an exclusive write lock on the region.
   * 
   * Use this when modifying chunk data (placing/breaking blocks).
   * Blocks all readers and other writers until the lock is released.
   * 
   * @return std::unique_lock<std::shared_mutex> Exclusive lock guard
   * 
   * Usage:
   * @code
   * auto writeLock = region->claimWriteLock();
   * // ... modify chunk blocks ...
   * // lock automatically released when writeLock goes out of scope
   * @endcode
   */
  std::unique_lock<std::shared_mutex> claimWriteLock();

  /**
   * @brief Claims a shared read lock on the region.
   * 
   * Use this when reading chunk data for mesh generation with cross-chunk
   * culling. Multiple readers can hold this lock simultaneously.
   * Blocks writers until all readers release their locks.
   * 
   * @return std::shared_lock<std::shared_mutex> Shared lock guard
   * 
   * Usage:
   * @code
   * auto readLock = region->claimReadLock();
   * // ... read chunk data for mesh generation ...
   * // lock automatically released when readLock goes out of scope
   * @endcode
   */
  std::shared_lock<std::shared_mutex> claimReadLock();

  // TODO: SVO generation for LOD

  Region();
  ~Region();

private:
  int localIndex(uint8_t x, uint8_t y, uint8_t z) const {
    return z + y * REGION_SIZE + x * REGION_SIZE * REGION_SIZE;
  }
  std::array<std::unique_ptr<Chunk>, REGION_VOLUME> m_chunks;
  std::shared_mutex m_mutex;
};

#endif // REGION_H
