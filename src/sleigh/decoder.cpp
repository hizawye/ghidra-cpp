#include "ghirda/sleigh/decoder.h"

namespace ghirda::sleigh {

DecodeResult Decoder::decode(const std::vector<uint8_t>& bytes, uint64_t address) {
  DecodeResult result{};
  if (bytes.empty()) {
    result.mnemonic = "invalid";
    return result;
  }

  result.mnemonic = "byte";
  PCodeOp op{};
  op.opcode = OpCode::Copy;
  op.output = Varnode{0, address, 1};
  op.inputs.push_back(Varnode{0, address, 1});
  result.pcode.push_back(op);
  return result;
}

} // namespace ghirda::sleigh
