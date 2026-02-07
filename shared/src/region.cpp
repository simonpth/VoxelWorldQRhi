#include "region.h"

Region::Region() {}

Region::~Region() {}

std::unique_lock<std::shared_mutex> Region::claimWriteLock() {
  return std::unique_lock<std::shared_mutex>(m_mutex);
}

std::shared_lock<std::shared_mutex> Region::claimReadLock() {
  return std::shared_lock<std::shared_mutex>(m_mutex);
}
