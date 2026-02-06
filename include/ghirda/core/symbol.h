#pragma once

#include <cstdint>
#include <string>

namespace ghirda::core {

enum class SymbolKind {
  Function,
  Label,
  Data,
  External,
  Unknown
};

struct Symbol {
  std::string name;
  uint64_t address = 0;
  SymbolKind kind = SymbolKind::Unknown;
};

} // namespace ghirda::core
