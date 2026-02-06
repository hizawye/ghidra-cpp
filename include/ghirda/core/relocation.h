#pragma once

#include <cstdint>
#include <string>

namespace ghirda::core {

struct Relocation {
  uint64_t address = 0;
  uint32_t type = 0;
  std::string symbol;
  int64_t addend = 0;
  bool applied = false;
  std::string note;
};

} // namespace ghirda::core
