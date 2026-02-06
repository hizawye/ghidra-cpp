#include "ghirda/core/memory_map.h"

namespace ghirda::core {

void MemoryMap::add_region(const MemoryRegion& region) { regions_.push_back(region); }

const std::vector<MemoryRegion>& MemoryMap::regions() const { return regions_; }

} // namespace ghirda::core
