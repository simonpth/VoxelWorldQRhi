#ifndef CHUNKMESHGENERATOR_H
#define CHUNKMESHGENERATOR_H

#include "chunk.h"
#include <QDebug>
#include <QtCore/qtypes.h>
#include <array>
#include <cstdint>
#include <vector>

struct ChunkFaceMesh {
  std::vector<uint64_t> vertices;

  void reserve(size_t v) {
    vertices.reserve(v);
  }
};

struct ChunkMesh {
  std::array<ChunkFaceMesh, 6> faces;
};

class ChunkMeshGenerator {
public:
  static uint64_t getVertex(uint16_t id, uint8_t rlx, uint8_t drlx, uint8_t rly, uint8_t drly, 
                            uint8_t face, uint8_t x, uint8_t dx, uint8_t y, uint8_t dy, uint8_t z, uint8_t dz) {
    // Bit layout for uvec2 (two 32-bit uints):
    // Upper 32 bits (high):
    //   id:     16 bits (bits 0-15)
    //   rlx:     5 bits (bits 16-20)
    //   drlx:    3 bits (bits 21-23)
    //   rly:     5 bits (bits 24-28)
    //   drly:    3 bits (bits 29-31)
    // Lower 32 bits (low):
    //   face:    3 bits (bits 0-2)   [0-5: x+,y+,z+,x-,y-,z-]
    //   dx:      3 bits (bits 3-5)
    //   x:       6 bits (bits 6-11)  [0-63 chunk position]
    //   dy:      3 bits (bits 12-14)
    //   y:       6 bits (bits 15-20) [0-63 chunk position]
    //   dz:      3 bits (bits 21-23)
    //   z:       6 bits (bits 24-29) [0-63 chunk position]
    //   unused:  2 bits (bits 30-31)

    // High word packing
    uint64_t high = ((uint64_t)(id & 0xFFFF)) 
                  | ((uint64_t)(rlx & 0x1F) << 16) 
                  | ((uint64_t)(drlx & 0x7) << 21)
                  | ((uint64_t)(rly & 0x1F) << 24) 
                  | ((uint64_t)(drly & 0x7) << 29);

    // Low word packing - coordinates fit in 6+3 bits (chunk is 32x32x32, but allow start at 32)
    uint64_t low = ((uint64_t)(face & 0x7))         // 3 bits for face (0-5)
                 | ((uint64_t)(dx & 0x7) << 3)      // 3 bits
                 | ((uint64_t)(x & 0x3F) << 6)      // 6 bits
                 | ((uint64_t)(dy & 0x7) << 12)     // 3 bits  
                 | ((uint64_t)(y & 0x3F) << 15)     // 6 bits
                 | ((uint64_t)(dz & 0x7) << 21)     // 3 bits
                 | ((uint64_t)(z & 0x3F) << 24);    // 6 bits

    return (high << 32) | low;
  }

  static std::unique_ptr<ChunkMesh> generateChunkMesh(const Chunk *chunk);

private:
  static ChunkFaceMesh generateFaceMesh(const Chunk *chunk, uint8_t face);
};

#endif // CHUNKMESHGENERATOR_H