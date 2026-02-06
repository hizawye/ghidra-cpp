#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ghirda::core {

struct DebugLineEntry {
  uint64_t address = 0;
  std::string file;
  uint32_t line = 0;
};

struct DebugFunction {
  std::string name;
  uint64_t low_pc = 0;
  uint64_t high_pc = 0;
  uint64_t return_type_ref = 0;
};

struct DebugMember {
  std::string name;
  uint64_t type_ref = 0;
  uint64_t offset = 0;
  uint32_t bit_size = 0;
  int32_t bit_offset = -1;
  uint32_t alignment = 0;
};

enum class DebugTypeKind {
  Base,
  Pointer,
  Struct,
  Array,
  Typedef,
  Union,
  Const,
  Volatile,
  Enumeration,
  Subroutine,
  Unknown
};

struct DebugType {
  std::string name;
  DebugTypeKind kind = DebugTypeKind::Unknown;
  uint32_t size = 0;
  uint64_t die_offset = 0;
  uint64_t type_ref = 0;
  uint64_t array_count = 0;
  std::vector<DebugMember> members;
};

struct DebugInfo {
  std::vector<DebugFunction> functions;
  std::vector<DebugLineEntry> lines;
  std::vector<DebugType> types;
  std::string pdb_path;
};

} // namespace ghirda::core
