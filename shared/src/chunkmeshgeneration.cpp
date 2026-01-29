#include "chunkmeshgeneration.h"
#include "block.h"
#include <QtCore/qlogging.h>

// TODO: for now only face culling

std::unique_ptr<ChunkMesh>
ChunkMeshGenerator::generateChunkMesh(const Chunk &chunk) {
  std::unique_ptr<ChunkMesh> chunkMesh = std::make_unique<ChunkMesh>();
  for (uint8_t face = 0; face < 6; face++) {
    chunkMesh->faces[face] = generateFaceMesh(chunk, face);
  }
  return std::move(chunkMesh);
}

static const uint8_t quadOffsets[6][4][3] = {
    // +X (right)
    {{1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 0}},

    // +Y (top)
    {{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}},

    // +Z (front)
    {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},

    // -X (left)
    {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}},

    // -Y (bottom)
    {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}},

    // -Z (back)
    {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}}};

static void addQuad(ChunkFaceMesh &mesh, uint32_t blockId, uint8_t x, uint8_t y,
                    uint8_t z, uint8_t face, uint8_t runLength) {
  uint32_t baseIndex = static_cast<uint32_t>(mesh.vertices.size());

  for (uint8_t i = 0; i < 4; i++) {
    uint8_t vx = x + quadOffsets[face][i][0];
    uint8_t vy = y + quadOffsets[face][i][1];
    uint8_t vz = z + quadOffsets[face][i][2];
    if (i == 1 || i == 2) {
      if (face == 0 || face == 3) {
        vx += runLength - 1;
      } else if (face == 1 || face == 4) {
        vy += runLength - 1;
      } else {
        vz += runLength - 1;
      }
    }
    mesh.vertices.push_back(
        ChunkMeshGenerator::getVertex(blockId, vx, vy, vz, face, 0, 0, 0));
  }

  mesh.indices.push_back(baseIndex + 0);
  mesh.indices.push_back(baseIndex + 1);
  mesh.indices.push_back(baseIndex + 2);
  mesh.indices.push_back(baseIndex + 0);
  mesh.indices.push_back(baseIndex + 2);
  mesh.indices.push_back(baseIndex + 3);
}

static const int faceOffsets[6][3] = {
    {1, 0, 0},  // x+
    {0, 1, 0},  // y+
    {0, 0, 1},  // z+
    {-1, 0, 0}, // x-
    {0, -1, 0}, // y-
    {0, 0, -1}  // z-
};

ChunkFaceMesh ChunkMeshGenerator::generateFaceMesh(const Chunk &chunk,
                                                   uint8_t face) {
  ChunkFaceMesh faceMesh;

  for (uint8_t i = 0; i < Chunk::CHUNK_SIZE; i++) {
    for (uint8_t j = 0; j < Chunk::CHUNK_SIZE; j++) {
      for (uint8_t k = 0; k < Chunk::CHUNK_SIZE; k++) {
        const Block &block = chunk.getBlock((int)i, (int)j, (int)k);
        if (block.isSolid()) {
          int nx = i + faceOffsets[face][0];
          int ny = j + faceOffsets[face][1];
          int nz = k + faceOffsets[face][2];
          if (nx >= 0 && nx < Chunk::CHUNK_SIZE && ny >= 0 &&
              ny < Chunk::CHUNK_SIZE && nz >= 0 && nz < Chunk::CHUNK_SIZE) {
            const Block &neighborBlock = chunk.getBlock(nx, ny, nz);
            if (neighborBlock.isSolid()) {
              continue;
            }
          }
          addQuad(faceMesh, block.id, i, j, k, face, 1);
        } else {
          // TODO: add slabs/stairs and detailed block meshing
        }
      }
    }
  }

  return faceMesh;
}

/*

// Precomputed: which axis is the "w" (scanning) axis for each face
// face 0,3 -> axis 0 (x), face 1,4 -> axis 1 (y), face 2,5 -> axis 2 (z)
static const int faceAxis[6] = {0, 1, 2, 0, 1, 2};

// axis 0: w=x, u=y, v=z | axis 1: u=x, w=y, v=z | axis 2: u=x, v=y, w=z
static const int mapping[3][3] = {
    {2, 0, 1}, {0, 2, 1}, {0, 1, 2}}; // [axis][xyz] -> uvw index

// Convert (u, v, w) to (x, y, z) based on face axis
static inline void uvwToXYZ(int u, int v, int w, int face, int &x, int &y,
                            int &z) {
  int axis = faceAxis[face];
  int coords[3] = {u, v, w};
  x = coords[mapping[axis][0]];
  y = coords[mapping[axis][1]];
  z = coords[mapping[axis][2]];
}

ChunkFaceMesh ChunkMeshGenerator::generateFaceMesh(const Chunk &chunk,
                                                   int face) {
  ChunkFaceMesh faceMesh;
  int dx = faceOffsets[face][0], dy = faceOffsets[face][1],
      dz = faceOffsets[face][2];

  for (int u = 0; u < Chunk::CHUNK_SIZE; u++) {
    for (int v = 0; v < Chunk::CHUNK_SIZE; v++) {
      int runStartW = -1;
      uint32_t runId = 0;

      for (int w = 0; w <= Chunk::CHUNK_SIZE; w++) {
        bool shouldAddFace = false;
        uint32_t currentBlockId = 0;

        if (w < Chunk::CHUNK_SIZE) {
          int x, y, z;
          uvwToXYZ(u, v, w, face, x, y, z);
          const Block &block = chunk.getBlock(x, y, z);

          if (block.isSolid()) {
            int nx = x + dx, ny = y + dy, nz = z + dz;
            bool neighborIsSolid =
                (nx >= 0 && nx < Chunk::CHUNK_SIZE && ny >= 0 &&
                 ny < Chunk::CHUNK_SIZE && nz >= 0 && nz < Chunk::CHUNK_SIZE) &&
                chunk.getBlock(nx, ny, nz).isSolid();
            if (!neighborIsSolid) {
              shouldAddFace = true;
              currentBlockId = block.id;
            }
          } else {
            // logic for non-solid blocks
            // TODO
            // get model by id and create vertices and indices
          }
        }

        // for greedy meshing: terminate run if block is not solid or block id
        // changes
        if (shouldAddFace && runStartW == -1) {
          runStartW = w;
          runId = currentBlockId;
        } else if (runStartW != -1 &&
                   (!shouldAddFace || runId != currentBlockId)) {
          int x, y, z;
          uvwToXYZ(u, v, runStartW, face, x, y, z);
          addQuad(faceMesh, runId, x, y, z, face, w - runStartW);
          runStartW = shouldAddFace ? w : -1;
          runId = currentBlockId;
        }
      }
    }
  }

  // TODO: add detailed block meshing

  return faceMesh;
}
*/