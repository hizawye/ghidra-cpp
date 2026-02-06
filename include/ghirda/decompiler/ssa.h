#pragma once

#include <cstdint>
#include <vector>

namespace ghirda::decompiler {

struct SSAValue {
  uint64_t id = 0;
  uint32_t size = 0;
};

class SSAGraph {
public:
  void add_value(const SSAValue& value);
  const std::vector<SSAValue>& values() const;

private:
  std::vector<SSAValue> values_{};
};

} // namespace ghirda::decompiler
