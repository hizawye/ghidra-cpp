#include "ghirda/core/program.h"

namespace ghirda::core {

Program::Program(std::string name) : name_(std::move(name)) {}

const std::string& Program::name() const { return name_; }

MemoryMap& Program::memory_map() { return memory_map_; }
const MemoryMap& Program::memory_map() const { return memory_map_; }

MemoryImage& Program::memory_image() { return memory_image_; }
const MemoryImage& Program::memory_image() const { return memory_image_; }

void Program::add_address_space(const AddressSpace& space) { address_spaces_.push_back(space); }
const std::vector<AddressSpace>& Program::address_spaces() const { return address_spaces_; }

void Program::add_symbol(const Symbol& symbol) { symbols_.push_back(symbol); }
const std::vector<Symbol>& Program::symbols() const { return symbols_; }

TypeSystem& Program::types() { return types_; }
const TypeSystem& Program::types() const { return types_; }

void Program::add_relocation(const Relocation& relocation) { relocations_.push_back(relocation); }
const std::vector<Relocation>& Program::relocations() const { return relocations_; }

void Program::set_load_bias(uint64_t bias) { load_bias_ = bias; }
uint64_t Program::load_bias() const { return load_bias_; }

DebugInfo& Program::debug_info() { return debug_info_; }
const DebugInfo& Program::debug_info() const { return debug_info_; }

void Program::add_section(const Section& section) { sections_.push_back(section); }
const std::vector<Program::Section>& Program::sections() const { return sections_; }

void Program::add_segment(const Segment& segment) { segments_.push_back(segment); }
const std::vector<Program::Segment>& Program::segments() const { return segments_; }

} // namespace ghirda::core
