#ifndef BLOCK_H
#define BLOCK_H

#include <array>
#include <cstdint>

struct Block {
  uint16_t id;   // block id
  uint16_t meta; // block metadata; 3 bits for rotation

  Block(uint16_t id = 0, uint16_t meta = 0) : id(id), meta(meta) {}

  bool isSolid() const {
    return id != 0 && id <= 0x7FFF;
    // 0 = air; 1-32767 = solid blocks; 32768-65535 = non-solid blocks
  }
};

struct DetailedBlock {
  std::array<uint16_t, 512> data;

  uint16_t getData(int x, int y, int z) const {
    return data[z + y * 8 + x * 64];
  }
};

#endif // BLOCK_H