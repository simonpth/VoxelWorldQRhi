#include "blockdefinitionlookup.h"

BlockDefinitionLookup::BlockDefinitionLookup() {
  // FOR NOW ONLY SOLID BLOCKS
  // LATER WE NEED LOGIC TO READ FROM FILE
  m_blockDefinitions.insert({0, BlockDefinition{"air"}});
  m_blockDefinitions.insert({1, BlockDefinition{"stone"}});
  m_blockDefinitions.insert({2, BlockDefinition{"grass"}});
  m_blockDefinitions.insert({3, BlockDefinition{"dirt"}});
  m_blockDefinitions.insert({4, BlockDefinition{"cobblestone"}});
  m_blockDefinitions.insert({5, BlockDefinition{"planks"}});
}

BlockDefinitionLookup::~BlockDefinitionLookup() {}

BlockDefinitionLookup &BlockDefinitionLookup::instance() {
  static BlockDefinitionLookup instance;
  return instance;
}

const BlockDefinition &
BlockDefinitionLookup::getBlockDefinition(uint16_t id) const {
  return m_blockDefinitions.at(id);
}