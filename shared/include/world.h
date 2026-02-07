#ifndef WORLD_H
#define WORLD_H

#include "region.h"

#include <QString>
#include <QtGui/qvectornd.h>
#include <memory>
#include <unordered_map>

struct RegionPos {
  // limited by WorldChunkPos
  int64_t x, y, z;

  bool operator==(const RegionPos &other) const {
    return x == other.x && y == other.y && z == other.z;
  }

  RegionPos operator+(const RegionPos &other) const {
    return RegionPos(x + other.x, y + other.y, z + other.z);
  }

  RegionPos operator-(const RegionPos &other) const {
    return RegionPos(x - other.x, y - other.y, z - other.z);
  }

  RegionPos(int64_t x, int64_t y, int64_t z) : x(x), y(y), z(z) {}

  RegionPos addHorizontal(const RegionPos &other) const {
    return RegionPos(x + other.x, y, z + other.z);
  }
};

struct RegionPosHash {
  size_t operator()(const RegionPos &pos) const {
    // Golden ratio hashing - excellent bit dispersion
    constexpr size_t golden = 0x9e3779b97f4a7c15ULL; // 2^64 / φ

    size_t hash = static_cast<size_t>(pos.x);
    hash ^= static_cast<size_t>(pos.y) + golden + (hash << 6) + (hash >> 2);
    hash ^= static_cast<size_t>(pos.z) + golden + (hash << 6) + (hash >> 2);
    return hash;
  }
};

struct WorldChunkPos {
  // every chunk gets its own position in the world
  // THIS IS THE LIMITING FACTOR FOR THE WORLD SIZE
  int64_t x, y, z;

  bool operator==(const WorldChunkPos &other) const {
    return x == other.x && y == other.y && z == other.z;
  }

  WorldChunkPos(int64_t x, int64_t y, int64_t z) : x(x), y(y), z(z) {}

  WorldChunkPos(const RegionPos &regionPos, const ChunkPos &chunkPos) {
    x = regionPos.x * Region::REGION_SIZE + chunkPos.x;
    y = regionPos.y * Region::REGION_SIZE + chunkPos.y;
    z = regionPos.z * Region::REGION_SIZE + chunkPos.z;
  }
};

struct WorldChunkPosHash {
  size_t operator()(const WorldChunkPos &pos) const {
    // Golden ratio hashing - excellent bit dispersion
    constexpr size_t golden = 0x9e3779b97f4a7c15ULL; // 2^64 / φ

    size_t hash = static_cast<size_t>(pos.x);
    hash ^= static_cast<size_t>(pos.y) + golden + (hash << 6) + (hash >> 2);
    hash ^= static_cast<size_t>(pos.z) + golden + (hash << 6) + (hash >> 2);
    return hash;
  }
};

struct PlayerWorldChunkPos {
  int64_t x, y, z;

  PlayerWorldChunkPos(int64_t x = 0, int64_t y = 0, int64_t z = 0)
      : x(x), y(y), z(z) {}

  PlayerWorldChunkPos(const RegionPos &regionPos, const ChunkPos &chunkPos) {
    x = regionPos.x * Region::REGION_SIZE + chunkPos.x;
    y = regionPos.y * Region::REGION_SIZE + chunkPos.y;
    z = regionPos.z * Region::REGION_SIZE + chunkPos.z;
  }

  bool operator==(const PlayerWorldChunkPos &other) const {
    return x == other.x && y == other.y && z == other.z;
  }

  PlayerWorldChunkPos operator+(const PlayerWorldChunkPos &other) const {
    return PlayerWorldChunkPos(x + other.x, y + other.y, z + other.z);
  }

  PlayerWorldChunkPos operator-(const PlayerWorldChunkPos &other) const {
    return PlayerWorldChunkPos(x - other.x, y - other.y, z - other.z);
  }

  PlayerWorldChunkPos operator+=(const PlayerWorldChunkPos &other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }

  PlayerWorldChunkPos operator-=(const PlayerWorldChunkPos &other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }

  QString toString() const { return QString("%1 %2 %3").arg(x).arg(y).arg(z); }
};

struct PlayerPos {
  PlayerWorldChunkPos playerWorldChunkPos;
  QVector3D localPlayerPosition;
  QVector3D cameraRotation;
};

class World {
public:
  World();
  ~World();

  const Region *region(const RegionPos &pos) {
    std::lock_guard<std::mutex> lock(m_regionsMutex);
    auto it = m_regions.find(pos);
    if (it != m_regions.end()) {
      return it->second.get();
    }
    return nullptr;
  }
  void setRegion(const RegionPos &pos, std::unique_ptr<Region> region) {
    std::lock_guard<std::mutex> lock(m_regionsMutex);
    m_regions[pos] = std::move(region);
  }

  const Region *generateRegion(const RegionPos &pos);

private:
  std::mutex m_regionsMutex;
  std::unordered_map<RegionPos, std::unique_ptr<Region>, RegionPosHash>
      m_regions;
};

#endif // WORLD_H