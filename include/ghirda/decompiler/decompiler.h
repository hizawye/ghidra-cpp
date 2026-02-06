#pragma once

#include <string>

#include "ghirda/core/program.h"

namespace ghirda::decompiler {

struct DecompileResult {
  std::string c_code;
  bool success = false;
};

class Decompiler {
public:
  DecompileResult decompile_function(const ghirda::core::Program& program, uint64_t entry);
};

} // namespace ghirda::decompiler
