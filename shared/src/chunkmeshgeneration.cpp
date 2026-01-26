#include "chunkmeshgeneration.h"
#include "block.h"

ChunkMesh ChunkMeshGenerator::generateChunkMesh(const Chunk &chunk) {
  ChunkMesh chunkMesh;
  for (int face = 0; face < 6; face++) {
    chunkMesh.faces[face] = generateFaceMesh(chunk, face);
  }
  return chunkMesh;
}

static const int faceOffsets[6][3] = {
    {1, 0, 0},  // x+
    {0, 1, 0},  // y+
    {0, 0, 1},  // z+
    {-1, 0, 0}, // x-
    {0, -1, 0}, // y-
    {0, 0, -1}  // z-
};

// Precomputed: which axis is the "w" (scanning) axis for each face
// face 0,3 -> axis 0 (x), face 1,4 -> axis 1 (y), face 2,5 -> axis 2 (z)
static const int faceAxis[6] = {0, 1, 2, 0, 1, 2};

// Precomputed quad vertex offsets for each face [4 corners][3 coords]
static const int quadOffsets[6][4][3] = {
    {{1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 0}}, // x+
    {{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}}, // y+
    {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}, // z+
    {{0, 0, 1}, {0, 0, 0}, {0, 1, 0}, {0, 1, 1}}, // x-
    {{0, 0, 1}, {1, 0, 1}, {1, 0, 0}, {0, 0, 0}}, // y-
    {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}}  // z-
};

// axis 0: w=x, u=y, v=z | axis 1: u=x, w=y, v=z | axis 2: u=x, v=y, w=z
static const int mapping[3][3] = {
    {2, 0, 1}, {0, 2, 1}, {0, 1, 2}}; // [axis][xyz] -> uvw index

static void addQuad(ChunkFaceMesh &mesh, uint32_t blockId, int x, int y, int z,
                    int face, int runLength) {
  uint32_t baseIndex = static_cast<uint32_t>(mesh.vertices.size());
  int pos[3] = {x, y, z};

  for (int i = 0; i < 4; i++) {
    mesh.vertices.push_back(ChunkMeshGenerator::getVertex(
        blockId, pos[0] + quadOffsets[face][i][0],
        pos[1] + quadOffsets[face][i][1], pos[2] + quadOffsets[face][i][2],
        face, 0, 0, 0));
  }

  mesh.indices.push_back(baseIndex + 0);
  mesh.indices.push_back(baseIndex + 1);
  mesh.indices.push_back(baseIndex + 2);
  mesh.indices.push_back(baseIndex + 0);
  mesh.indices.push_back(baseIndex + 2);
  mesh.indices.push_back(baseIndex + 3);
}

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