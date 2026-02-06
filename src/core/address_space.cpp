#include "ghirda/core/address_space.h"

namespace ghirda::core {

AddressSpace::AddressSpace(std::string name, uint64_t base, uint64_t size)
    : name_(std::move(name)), base_(base), size_(size) {}

const std::string& AddressSpace::name() const { return name_; }
uint64_t AddressSpace::base() const { return base_; }
uint64_t AddressSpace::size() const { return size_; }

} // namespace ghirda::core
