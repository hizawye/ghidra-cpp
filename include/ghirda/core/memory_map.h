#pragma once

#include <cstdint>
#include <vector>

namespace ghirda::core {

struct MemoryRegion {
  uint64_t start = 0;
  uint64_t size = 0;
  bool readable = false;
  bool writable = false;
  bool executable = false;
};

class MemoryMap {
public:
  void add_region(const MemoryRegion& region);
  const std::vector<MemoryRegion>& regions() const;

private:
  std::vector<MemoryRegion> regions_;
};

} // namespace ghirda::core
