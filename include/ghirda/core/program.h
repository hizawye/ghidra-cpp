#pragma once

#include <string>
#include <vector>

#include "ghirda/core/address_space.h"
#include "ghirda/core/memory_map.h"
#include "ghirda/core/relocation.h"
#include "ghirda/core/memory_image.h"
#include "ghirda/core/symbol.h"
#include "ghirda/core/type_system.h"

namespace ghirda::core {

class Program {
public:
  explicit Program(std::string name);

  const std::string& name() const;
  MemoryMap& memory_map();
  const MemoryMap& memory_map() const;
  MemoryImage& memory_image();
  const MemoryImage& memory_image() const;

  void add_address_space(const AddressSpace& space);
  const std::vector<AddressSpace>& address_spaces() const;

  void add_symbol(const Symbol& symbol);
  const std::vector<Symbol>& symbols() const;

  TypeSystem& types();
  const TypeSystem& types() const;

  void add_relocation(const Relocation& relocation);
  const std::vector<Relocation>& relocations() const;

  void set_load_bias(uint64_t bias);
  uint64_t load_bias() const;

private:
  std::string name_;
  MemoryMap memory_map_{};
  MemoryImage memory_image_{};
  std::vector<AddressSpace> address_spaces_{};
  std::vector<Symbol> symbols_{};
  TypeSystem types_{};
  std::vector<Relocation> relocations_{};
  uint64_t load_bias_ = 0;
};

} // namespace ghirda::core
