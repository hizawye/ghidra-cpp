#pragma once

#include <string>
#include <vector>

namespace ghirda::plugin {

struct PluginInfo {
  std::string id;
  std::string name;
};

class Registry {
public:
  void register_plugin(const PluginInfo& info);
  const std::vector<PluginInfo>& plugins() const;

private:
  std::vector<PluginInfo> plugins_{};
};

} // namespace ghirda::plugin
