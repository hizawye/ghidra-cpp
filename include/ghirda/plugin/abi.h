#pragma once

#include <cstdint>

namespace ghirda::plugin {

struct PluginAbi {
  uint32_t major = 0;
  uint32_t minor = 1;
};

} // namespace ghirda::plugin
