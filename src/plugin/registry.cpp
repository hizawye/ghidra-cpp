#include "ghirda/plugin/registry.h"

namespace ghirda::plugin {

void Registry::register_plugin(const PluginInfo& info) { plugins_.push_back(info); }

const std::vector<PluginInfo>& Registry::plugins() const { return plugins_; }

} // namespace ghirda::plugin
