#pragma once

#include <string>

namespace ghirda::script {

struct ScriptApiVersion {
  int major = 0;
  int minor = 1;
};

class ScriptApi {
public:
  ScriptApiVersion version() const;
  void register_binding(const std::string& name);
};

} // namespace ghirda::script
