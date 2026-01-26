#ifndef WORLD_H
#define WORLD_H

#include "region.h"
#include <memory>
#include <unordered_map>

struct RegionPos {
  int64_t x, y, z;

  bool operator==(const RegionPos &other) const {
    return x == other.x && y == other.y && z == other.z;
  }

  RegionPos(int64_t x, int64_t y, int64_t z) : x(x), y(y), z(z) {}
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

class World {
public:
  World();
  ~World();

  const Region *getRegion(const RegionPos &pos) const {
    auto it = m_regions.find(pos);
    if (it != m_regions.end()) {
      return it->second.get();
    }
    return nullptr;
  }
  void setRegion(const RegionPos &pos, std::unique_ptr<Region> region) {
    m_regions[pos] = std::move(region);
  }

  void generateRegion(const RegionPos &pos);
  void basicSetup() { generateRegion({0, 0, 0}); }

private:
  std::unordered_map<RegionPos, std::unique_ptr<Region>, RegionPosHash>
      m_regions;
};

#endif // WORLD_H