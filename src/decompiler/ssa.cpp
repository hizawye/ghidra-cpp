#include "ghirda/decompiler/ssa.h"

namespace ghirda::decompiler {

void SSAGraph::add_value(const SSAValue& value) { values_.push_back(value); }
const std::vector<SSAValue>& SSAGraph::values() const { return values_; }

} // namespace ghirda::decompiler
