#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ghirda::core {

enum class TypeKind {
  Void,
  Integer,
  Float,
  Pointer,
  Struct,
  Array
};

struct Type {
  TypeKind kind = TypeKind::Void;
  std::string name;
  uint32_t size = 0;
};

class TypeSystem {
public:
  void add_type(const Type& type);
  const std::vector<Type>& types() const;

private:
  std::vector<Type> types_;
};

} // namespace ghirda::core
