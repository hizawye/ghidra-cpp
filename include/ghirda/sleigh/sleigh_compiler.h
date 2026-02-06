#pragma once

#include <string>
#include <vector>

namespace ghirda::sleigh {

struct SleighSpec {
  std::string name;
  std::string source_path;
};

class SleighCompiler {
public:
  bool compile(const SleighSpec& spec, std::string* error);
  const std::vector<std::string>& warnings() const;

private:
  std::vector<std::string> warnings_{};
};

} // namespace ghirda::sleigh
