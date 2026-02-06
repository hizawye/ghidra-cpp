#pragma once

#include <string>

namespace ghirda::script {

class LuaRuntime {
public:
  bool initialize(std::string* error);
  bool run_file(const std::string& path, std::string* error);
};

} // namespace ghirda::script
