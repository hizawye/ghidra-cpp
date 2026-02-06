#pragma once

#include <string>
#include <vector>

#include "ghirda/core/address_space.h"
#include "ghirda/core/debug_info.h"
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

  DebugInfo& debug_info();
  const DebugInfo& debug_info() const;

  struct Section {
    std::string name;
    uint64_t address = 0;
    uint64_t size = 0;
    uint64_t file_offset = 0;
    uint64_t flags = 0;
  };

  struct Segment {
    uint64_t vaddr = 0;
    uint64_t memsz = 0;
    uint64_t filesz = 0;
    uint64_t flags = 0;
  };

  void add_section(const Section& section);
  const std::vector<Section>& sections() const;
  void add_segment(const Segment& segment);
  const std::vector<Segment>& segments() const;

private:
  std::string name_;
  MemoryMap memory_map_{};
  MemoryImage memory_image_{};
  std::vector<AddressSpace> address_spaces_{};
  std::vector<Symbol> symbols_{};
  TypeSystem types_{};
  std::vector<Relocation> relocations_{};
  uint64_t load_bias_ = 0;
  DebugInfo debug_info_{};
  std::vector<Section> sections_{};
  std::vector<Segment> segments_{};
};

} // namespace ghirda::core
