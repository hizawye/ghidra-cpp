#include "ghirda/core/type_system.h"

namespace ghirda::core {

void TypeSystem::add_type(const Type& type) { types_.push_back(type); }

const std::vector<Type>& TypeSystem::types() const { return types_; }

} // namespace ghirda::core
