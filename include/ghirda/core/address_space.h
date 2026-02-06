#pragma once

#include <cstdint>
#include <string>

namespace ghirda::core {

class AddressSpace {
public:
  AddressSpace(std::string name, uint64_t base, uint64_t size);

  const std::string& name() const;
  uint64_t base() const;
  uint64_t size() const;

private:
  std::string name_;
  uint64_t base_;
  uint64_t size_;
};

} // namespace ghirda::core
