#include "chunkgeneration.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

namespace {
const std::vector<int> &getPermutation() {
  static const std::vector<int> p = []() {
    std::vector<int> p(512);
    std::iota(p.begin(), p.begin() + 256, 0);
    std::default_random_engine engine(42);
    std::shuffle(p.begin(), p.begin() + 256, engine);
    for (int i = 0; i < 256; ++i)
      p[256 + i] = p[i];
    return p;
  }();
  return p;
}

double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }

double lerp(double t, double a, double b) { return a + t * (b - a); }

double grad(int hash, double x, double y) {
  int h = hash & 15;
  double u = h < 8 ? x : y;
  double v = h < 4 ? y : h == 12 || h == 14 ? x : 0;
  return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double perlin(double x, double y) {
  const auto &p = getPermutation();

  int X = (int)std::floor(x) & 255;
  int Y = (int)std::floor(y) & 255;

  x -= std::floor(x);
  y -= std::floor(y);

  double u = fade(x);
  double v = fade(y);

  int A = p[X] + Y;
  int B = p[X + 1] + Y;

  return lerp(v, lerp(u, grad(p[A], x, y), grad(p[B], x - 1, y)),
              lerp(u, grad(p[A + 1], x, y - 1), grad(p[B + 1], x - 1, y - 1)));
}
} // namespace

// generate basic chunk with stone using 2D Perlin noise
std::unique_ptr<Chunk>
ChunkGenerator::generateChunk(const WorldChunkPos &chunkPos) {
  auto chunk = std::make_unique<Chunk>();

  const int groundLevel = 512;
  const double frequency = 0.01;
  const double amplitude = 64.0;

  int64_t startX = chunkPos.x * Chunk::CHUNK_SIZE;
  int64_t startY = chunkPos.y * Chunk::CHUNK_SIZE;
  int64_t startZ = chunkPos.z * Chunk::CHUNK_SIZE;

  for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
    for (int z = 0; z < Chunk::CHUNK_SIZE; z++) {

      double globalX = static_cast<double>(startX + x);
      double globalZ = static_cast<double>(startZ + z);

      double noise = perlin(globalX * frequency, globalZ * frequency);
      int64_t height = groundLevel + static_cast<int64_t>(noise * amplitude);

      for (int y = 0; y < Chunk::CHUNK_SIZE; y++) {
        int64_t globalY = startY + y;
        if (globalY <= height) {
          chunk->setBlock(x, y, z, Block(1)); // Stone
        } else {
          chunk->setBlock(x, y, z, Block(0)); // Air
        }
      }
    }
  }
  return chunk;
}