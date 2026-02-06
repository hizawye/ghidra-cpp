#include "ghirda/sleigh/sleigh_compiler.h"

namespace ghirda::sleigh {

bool SleighCompiler::compile(const SleighSpec& spec, std::string* error) {
  warnings_.clear();
  if (spec.source_path.empty()) {
    if (error) {
      *error = "missing sleigh source path";
    }
    return false;
  }
  return true;
}

const std::vector<std::string>& SleighCompiler::warnings() const { return warnings_; }

} // namespace ghirda::sleigh
