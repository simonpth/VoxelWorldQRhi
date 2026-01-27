#ifndef BLOCKDEFINITIONLOOKUP_H
#define BLOCKDEFINITIONLOOKUP_H

#include <cstdint>
#include <string>
#include <unordered_map>

struct BlockDefinition {
  std::string name;
  uint8_t r, g, b, a;
};

class BlockDefinitionLookup {
public:
  static BlockDefinitionLookup &instance();

  const BlockDefinition &getBlockDefinition(uint16_t id) const;

private:
  BlockDefinitionLookup();
  ~BlockDefinitionLookup();

  std::unordered_map<uint16_t, BlockDefinition> m_blockDefinitions;
};

#endif // BLOCKDEFINITIONLOOKUP_H