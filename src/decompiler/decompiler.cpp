#include "ghirda/decompiler/decompiler.h"

namespace ghirda::decompiler {

DecompileResult Decompiler::decompile_function(const ghirda::core::Program&, uint64_t) {
  return DecompileResult{"/* decompiler not implemented */", false};
}

} // namespace ghirda::decompiler
