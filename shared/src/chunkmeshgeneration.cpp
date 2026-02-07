#include "chunkmeshgeneration.h"
#include "block.h"
#include <QtCore/qlogging.h>

// TODO: for now only face culling

std::unique_ptr<ChunkMesh>
ChunkMeshGenerator::generateChunkMesh(const Chunk *chunk) {
  std::unique_ptr<ChunkMesh> chunkMesh = std::make_unique<ChunkMesh>();
  for (uint8_t face = 0; face < 6; face++) {
    chunkMesh->faces[face] = generateFaceMesh(chunk, face);
  }
  return std::move(chunkMesh);
}

static void addFullQuad(ChunkFaceMesh &mesh, uint32_t blockId, uint8_t x,
                        uint8_t y, uint8_t z, uint8_t face) {
  uint8_t originalFace = face;
  switch (face) {
  case 0: // x+
    x += 1;
    break;
  case 1: // y+
    y += 1;
    break;
  case 2: // z+
    z += 1;
    break;
  default:
    // For negative faces (3,4,5 = x-,y-,z-), no position adjustment needed
    // Face value is now passed as-is (0-5) for shader to handle
    break;
  }
  
  mesh.vertices.push_back(ChunkMeshGenerator::getVertex(
      blockId, 1, 0, 1, 0, originalFace, x, 0, y, 0, z, 0));
}

static const int faceOffsets[6][3] = {
    {1, 0, 0},  // x+
    {0, 1, 0},  // y+
    {0, 0, 1},  // z+
    {-1, 0, 0}, // x-
    {0, -1, 0}, // y-
    {0, 0, -1}  // z-
};

ChunkFaceMesh ChunkMeshGenerator::generateFaceMesh(const Chunk *chunk,
                                                   uint8_t face) {
  ChunkFaceMesh faceMesh;

  for (uint8_t i = 0; i < Chunk::CHUNK_SIZE; i++) {
    for (uint8_t j = 0; j < Chunk::CHUNK_SIZE; j++) {
      for (uint8_t k = 0; k < Chunk::CHUNK_SIZE; k++) {
        const Block &block = chunk->block((int)i, (int)j, (int)k);
        if (block.isSolid()) {
          int nx = i + faceOffsets[face][0];
          int ny = j + faceOffsets[face][1];
          int nz = k + faceOffsets[face][2];
          if (nx >= 0 && nx < Chunk::CHUNK_SIZE && ny >= 0 &&
              ny < Chunk::CHUNK_SIZE && nz >= 0 && nz < Chunk::CHUNK_SIZE) {
            const Block &neighborBlock = chunk->block(nx, ny, nz);
            if (neighborBlock.isSolid()) {
              continue;
            }
          }
          addFullQuad(faceMesh, block.id, i, j, k, face);
        } else {
          // TODO: add slabs/stairs and detailed block meshing
        }
      }
    }
  }

  return faceMesh;
}