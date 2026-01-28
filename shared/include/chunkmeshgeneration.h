#ifndef CHUNKMESHGENERATOR_H
#define CHUNKMESHGENERATOR_H

#include "chunk.h"
#include <array>
#include <cstdint>
#include <vector>

struct ChunkFaceMesh {
  std::vector<uint64_t> vertices;
  std::vector<uint32_t> indices;

  void reserve(size_t v, size_t i) {
    vertices.reserve(v);
    indices.reserve(i);
  }
};

struct ChunkMesh {
  std::array<ChunkFaceMesh, 6> faces;
};

class ChunkMeshGenerator {
public:
  static uint64_t getVertex(uint32_t id, uint8_t x, uint8_t y, uint8_t z,
                            uint8_t face, uint8_t detailedX, uint8_t detailedY,
                            uint8_t detailedZ) {
    // bit wise encoding
    // empty: 16 bits
    // id: 16 bits
    // face: 3 bits 6 values
    // x, y, z: 9 bits: 5 bits for 0-31 (x,y,z), 4 bits for 0.0-1.0 in 1/8th
    // (detailedX,detailedY,detailedZ)
    // 2 empty bits
    return (uint64_t)id << 32 | (uint64_t)face << 29 | (uint64_t)x << 24 |
           (uint64_t)detailedX << 20 | (uint64_t)y << 15 |
           (uint64_t)detailedY << 11 | (uint64_t)z << 6 |
           (uint64_t)detailedZ << 2;
  }

  static std::unique_ptr<ChunkMesh> generateChunkMesh(const Chunk &chunk);

private:
  static ChunkFaceMesh generateFaceMesh(const Chunk &chunk, int face);
};

#endif // CHUNKMESHGENERATOR_H