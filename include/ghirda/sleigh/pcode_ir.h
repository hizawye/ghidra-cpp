#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ghirda::sleigh {

enum class OpCode {
  Copy,
  Load,
  Store,
  Branch,
  Call,
  Return,
  Unknown
};

struct Varnode {
  uint64_t space = 0;
  uint64_t offset = 0;
  uint32_t size = 0;
};

struct PCodeOp {
  OpCode opcode = OpCode::Unknown;
  Varnode output{};
  std::vector<Varnode> inputs{};
};

} // namespace ghirda::sleigh
