#include "chunkmeshgeneration.h"
#include "block.h"
#include "region.h"
#include <array>
#include <cstdint>

namespace {
constexpr int kChunkSize = Chunk::CHUNK_SIZE;
constexpr int kPaddedSize = kChunkSize + 2;
constexpr int kMaxRun = 31;

using Column = std::array<std::array<uint64_t, kPaddedSize>, kPaddedSize>;
using FaceMasks = std::array<Column, 6>;

const Block &airBlock() {
  static Block air(0, 0);
  return air;
}

const Block &getBlockAt(const Chunk *chunk, const Region *region,
                        const ChunkPos &chunkPos, int x, int y, int z) {
  if (!chunk || !region) {
    return airBlock();
  }

  int cx = chunkPos.x;
  int cy = chunkPos.y;
  int cz = chunkPos.z;
  int lx = x;
  int ly = y;
  int lz = z;

  if (lx < 0) {
    cx -= 1;
    lx += kChunkSize;
  } else if (lx >= kChunkSize) {
    cx += 1;
    lx -= kChunkSize;
  }

  if (ly < 0) {
    cy -= 1;
    ly += kChunkSize;
  } else if (ly >= kChunkSize) {
    cy += 1;
    ly -= kChunkSize;
  }

  if (lz < 0) {
    cz -= 1;
    lz += kChunkSize;
  } else if (lz >= kChunkSize) {
    cz += 1;
    lz -= kChunkSize;
  }

  if (cx < 0 || cx >= Region::REGION_SIZE || cy < 0 ||
      cy >= Region::REGION_SIZE || cz < 0 || cz >= Region::REGION_SIZE) {
    return airBlock();
  }

  const Chunk *target = region->chunk(cx, cy, cz);
  if (!target) {
    return airBlock();
  }

  return target->block(lx, ly, lz);
}

FaceMasks buildFaceMasks(const Chunk *chunk, const Region *region,
                         const ChunkPos &chunkPos) {
  std::array<Column, 3> axisCols{};
  FaceMasks masks{};

  for (int z = 0; z < kPaddedSize; ++z) {
    for (int y = 0; y < kPaddedSize; ++y) {
      for (int x = 0; x < kPaddedSize; ++x) {
        const Block &block =
            getBlockAt(chunk, region, chunkPos, x - 1, y - 1, z - 1);
        if (!block.isSolid()) {
          continue;
        }
        axisCols[0][z][x] |= (1ULL << y); // y axis (x,z column)
        axisCols[1][y][z] |= (1ULL << x); // x axis (y,z column)
        axisCols[2][y][x] |= (1ULL << z); // z axis (x,y column)
      }
    }
  }

  for (int z = 0; z < kPaddedSize; ++z) {
    for (int x = 0; x < kPaddedSize; ++x) {
      uint64_t col = axisCols[0][z][x];
      masks[4][z][x] = col & ~(col << 1); // y-
      masks[1][z][x] = col & ~(col >> 1); // y+
    }
  }

  for (int y = 0; y < kPaddedSize; ++y) {
    for (int z = 0; z < kPaddedSize; ++z) {
      uint64_t col = axisCols[1][y][z];
      masks[3][y][z] = col & ~(col << 1); // x-
      masks[0][y][z] = col & ~(col >> 1); // x+
    }
  }

  for (int y = 0; y < kPaddedSize; ++y) {
    for (int x = 0; x < kPaddedSize; ++x) {
      uint64_t col = axisCols[2][y][x];
      masks[5][y][x] = col & ~(col << 1); // z-
      masks[2][y][x] = col & ~(col >> 1); // z+
    }
  }

  return masks;
}

void emitQuad(ChunkFaceMesh &mesh, uint16_t blockId, uint8_t face, int plane,
              int u, int v, int width, int height) {
  uint8_t rlx = static_cast<uint8_t>(width);
  uint8_t rly = static_cast<uint8_t>(height);
  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t z = 0;

  switch (face) {
  case 0: // x+
    x = static_cast<uint8_t>(plane + 1);
    y = static_cast<uint8_t>(u);
    z = static_cast<uint8_t>(v);
    break;
  case 3: // x-
    x = static_cast<uint8_t>(plane);
    y = static_cast<uint8_t>(v);
    z = static_cast<uint8_t>(u);
    break;
  case 1: // y+
    x = static_cast<uint8_t>(v);
    y = static_cast<uint8_t>(plane + 1);
    z = static_cast<uint8_t>(u);
    break;
  case 4: // y-
    x = static_cast<uint8_t>(u);
    y = static_cast<uint8_t>(plane);
    z = static_cast<uint8_t>(v);
    break;
  case 2: // z+
    x = static_cast<uint8_t>(u);
    y = static_cast<uint8_t>(v);
    z = static_cast<uint8_t>(plane + 1);
    break;
  default: // z-
    x = static_cast<uint8_t>(v);
    y = static_cast<uint8_t>(u);
    z = static_cast<uint8_t>(plane);
    break;
  }

  mesh.vertices.push_back(ChunkMeshGenerator::getVertex(
      blockId, rlx, 0, rly, 0, face, x, 0, y, 0, z, 0));
}

void greedyMeshPlane(ChunkFaceMesh &mesh, uint8_t face, int plane,
                     std::vector<uint16_t> &mask) {
  for (int v = 0; v < kChunkSize; ++v) {
    for (int u = 0; u < kChunkSize; ++u) {
      int index = u + v * kChunkSize;
      uint16_t blockId = mask[index];
      if (blockId == 0) {
        continue;
      }

      int width = 1;
      while (u + width < kChunkSize && width < kMaxRun &&
             mask[index + width] == blockId) {
        ++width;
      }

      int height = 1;
      bool done = false;
      while (v + height < kChunkSize && height < kMaxRun && !done) {
        int rowStart = (v + height) * kChunkSize + u;
        for (int du = 0; du < width; ++du) {
          if (mask[rowStart + du] != blockId) {
            done = true;
            break;
          }
        }
        if (!done) {
          ++height;
        }
      }

      emitQuad(mesh, blockId, face, plane, u, v, width, height);

      for (int dv = 0; dv < height; ++dv) {
        int rowStart = (v + dv) * kChunkSize + u;
        for (int du = 0; du < width; ++du) {
          mask[rowStart + du] = 0;
        }
      }
    }
  }
}

ChunkFaceMesh generateFaceMesh(const Chunk *chunk, uint8_t face,
                               const FaceMasks &masks) {
  ChunkFaceMesh faceMesh;
  std::vector<uint16_t> mask(kChunkSize * kChunkSize, 0);

  for (int plane = 0; plane < kChunkSize; ++plane) {
    std::fill(mask.begin(), mask.end(), 0);

    switch (face) {
    case 0: { // x+
      for (int y = 0; y < kChunkSize; ++y) {
        for (int z = 0; z < kChunkSize; ++z) {
          uint64_t col = masks[0][y + 1][z + 1];
          if (col & (1ULL << (plane + 1))) {
            const Block &block = chunk->block(plane, y, z);
            mask[y + z * kChunkSize] = block.id;
          }
        }
      }
      break;
    }
    case 3: { // x-
      for (int y = 0; y < kChunkSize; ++y) {
        for (int z = 0; z < kChunkSize; ++z) {
          uint64_t col = masks[3][y + 1][z + 1];
          if (col & (1ULL << (plane + 1))) {
            const Block &block = chunk->block(plane, y, z);
            mask[z + y * kChunkSize] = block.id;
          }
        }
      }
      break;
    }
    case 1: { // y+
      for (int x = 0; x < kChunkSize; ++x) {
        for (int z = 0; z < kChunkSize; ++z) {
          uint64_t col = masks[1][z + 1][x + 1];
          if (col & (1ULL << (plane + 1))) {
            const Block &block = chunk->block(x, plane, z);
            mask[z + x * kChunkSize] = block.id;
          }
        }
      }
      break;
    }
    case 4: { // y-
      for (int x = 0; x < kChunkSize; ++x) {
        for (int z = 0; z < kChunkSize; ++z) {
          uint64_t col = masks[4][z + 1][x + 1];
          if (col & (1ULL << (plane + 1))) {
            const Block &block = chunk->block(x, plane, z);
            mask[x + z * kChunkSize] = block.id;
          }
        }
      }
      break;
    }
    case 2: { // z+
      for (int x = 0; x < kChunkSize; ++x) {
        for (int y = 0; y < kChunkSize; ++y) {
          uint64_t col = masks[2][y + 1][x + 1];
          if (col & (1ULL << (plane + 1))) {
            const Block &block = chunk->block(x, y, plane);
            mask[x + y * kChunkSize] = block.id;
          }
        }
      }
      break;
    }
    default: { // z-
      for (int x = 0; x < kChunkSize; ++x) {
        for (int y = 0; y < kChunkSize; ++y) {
          uint64_t col = masks[5][y + 1][x + 1];
          if (col & (1ULL << (plane + 1))) {
            const Block &block = chunk->block(x, y, plane);
            mask[y + x * kChunkSize] = block.id;
          }
        }
      }
      break;
    }
    }

    greedyMeshPlane(faceMesh, face, plane, mask);
  }

  return faceMesh;
}
} // namespace

std::unique_ptr<ChunkMesh>
ChunkMeshGenerator::generateChunkMesh(const Chunk *chunk, const Region *region,
                                      const ChunkPos &chunkPos) {
  std::unique_ptr<ChunkMesh> chunkMesh = std::make_unique<ChunkMesh>();
  FaceMasks masks = buildFaceMasks(chunk, region, chunkPos);

  for (uint8_t face = 0; face < 6; ++face) {
    chunkMesh->faces[face] = generateFaceMesh(chunk, face, masks);
  }

  return chunkMesh;
}