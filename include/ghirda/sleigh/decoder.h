#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ghirda/sleigh/pcode_ir.h"

namespace ghirda::sleigh {

struct DecodeResult {
  std::string mnemonic;
  std::vector<PCodeOp> pcode;
};

class Decoder {
public:
  DecodeResult decode(const std::vector<uint8_t>& bytes, uint64_t address);
};

} // namespace ghirda::sleigh
