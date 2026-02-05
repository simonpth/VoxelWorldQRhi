#ifndef CHUNK_H
#define CHUNK_H

#include "block.h"
#include <array>
#include <cstdint>
#include <unordered_map>

class Chunk {
public:
  static constexpr int CHUNK_SIZE = 32;
  static constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

  const Block &block(int x, int y, int z) const {
    return m_blocks[localIndex(x, y, z)];
  }
  void setBlock(int x, int y, int z, const Block &block) {
    m_blocks[localIndex(x, y, z)] = block;
  }

  const DetailedBlock &detailedBlock(int x, int y, int z) const {
    return m_detailedBlocks.at(localDetailedIndex(x, y, z));
  }
  void setDetailedBlock(int x, int y, int z,
                        const DetailedBlock &detailedBlock) {
    m_detailedBlocks[localDetailedIndex(x, y, z)] = detailedBlock;
  }
  int detailedBlockCount() const { return m_detailedBlocks.size(); }

  Chunk();
  ~Chunk();

private:
  int localIndex(int x, int y, int z) const {
    return z + y * CHUNK_SIZE + x * CHUNK_SIZE * CHUNK_SIZE;
  }
  int localDetailedIndex(int x, int y, int z) const {
    return z + y * 8 + x * 64;
  }
  std::array<Block, CHUNK_VOLUME> m_blocks;
  // pos stored as uint16_t (8x8x8 detailed block position); 3 bits for each
  // axis (x,y,z)
  std::unordered_map<uint16_t, DetailedBlock> m_detailedBlocks;
};

#endif // CHUNK_H