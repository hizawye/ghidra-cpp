#include "ghirda/core/program.h"

namespace ghirda::core {

Program::Program(std::string name) : name_(std::move(name)) {}

const std::string& Program::name() const { return name_; }

MemoryMap& Program::memory_map() { return memory_map_; }
const MemoryMap& Program::memory_map() const { return memory_map_; }

void Program::add_address_space(const AddressSpace& space) { address_spaces_.push_back(space); }
const std::vector<AddressSpace>& Program::address_spaces() const { return address_spaces_; }

void Program::add_symbol(const Symbol& symbol) { symbols_.push_back(symbol); }
const std::vector<Symbol>& Program::symbols() const { return symbols_; }

TypeSystem& Program::types() { return types_; }
const TypeSystem& Program::types() const { return types_; }

} // namespace ghirda::core
