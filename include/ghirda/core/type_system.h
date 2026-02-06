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
  Array,
  Union
};

struct TypeMember {
  std::string name;
  std::string type_name;
  uint32_t offset = 0;
  uint32_t size = 0;
  uint32_t bit_size = 0;
  int32_t bit_offset = -1;
  uint32_t alignment = 0;
};

struct Type {
  TypeKind kind = TypeKind::Void;
  std::string name;
  uint32_t size = 0;
  std::vector<TypeMember> members;
};

class TypeSystem {
public:
  void add_type(const Type& type);
  const std::vector<Type>& types() const;

private:
  std::vector<Type> types_;
};

} // namespace ghirda::core
