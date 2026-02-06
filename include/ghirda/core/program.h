#pragma once

#include <string>
#include <vector>

#include "ghirda/core/address_space.h"
#include "ghirda/core/memory_map.h"
#include "ghirda/core/symbol.h"
#include "ghirda/core/type_system.h"

namespace ghirda::core {

class Program {
public:
  explicit Program(std::string name);

  const std::string& name() const;
  MemoryMap& memory_map();
  const MemoryMap& memory_map() const;

  void add_address_space(const AddressSpace& space);
  const std::vector<AddressSpace>& address_spaces() const;

  void add_symbol(const Symbol& symbol);
  const std::vector<Symbol>& symbols() const;

  TypeSystem& types();
  const TypeSystem& types() const;

private:
  std::string name_;
  MemoryMap memory_map_{};
  std::vector<AddressSpace> address_spaces_{};
  std::vector<Symbol> symbols_{};
  TypeSystem types_{};
};

} // namespace ghirda::core
