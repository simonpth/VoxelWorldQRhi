#ifndef CHUNKMESHGENERATOR_H
#define CHUNKMESHGENERATOR_H

#include "chunk.h"
#include <QDebug>
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
  static uint64_t getVertex(uint16_t id, uint8_t x, uint8_t y, uint8_t z,
                            uint8_t face, uint8_t detailedX, uint8_t detailedY,
                            uint8_t detailedZ) {
    // New bit wise encoding:
    // Upper 32 bits (high):
    //   id: 16 bits (bits 0-15)
    //   face: 3 bits (bits 16-18)
    // Lower 32 bits (low):
    //   x: 10 bits (bits 20-29) -> 6 bits integer (0-63), 4 bits fractional
    //   (0-15) y: 10 bits (bits 10-19) -> 6 bits integer (0-63), 4 bits
    //   fractional (0-15) z: 10 bits (bits 0-9)   -> 6 bits integer (0-63), 4
    //   bits fractional (0-15)

    uint64_t high = ((uint64_t)id & 0xFFFF) | (((uint64_t)face & 0x7) << 16);

    // Each coordinate is 10 bits: 6 bits integer, 4 bits fractional
    uint64_t packX = (((uint64_t)x & 0x3F) << 4) | ((uint64_t)detailedX & 0xF);
    uint64_t packY = (((uint64_t)y & 0x3F) << 4) | ((uint64_t)detailedY & 0xF);
    uint64_t packZ = (((uint64_t)z & 0x3F) << 4) | ((uint64_t)detailedZ & 0xF);

    uint64_t low = (packX << 20) | (packY << 10) | packZ;

    return (high << 32) | low;
  }

  static std::unique_ptr<ChunkMesh> generateChunkMesh(const Chunk *chunk);

private:
  static ChunkFaceMesh generateFaceMesh(const Chunk *chunk, uint8_t face);
};

#endif // CHUNKMESHGENERATOR_H