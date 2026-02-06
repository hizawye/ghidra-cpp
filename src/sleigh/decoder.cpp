#include "ghirda/sleigh/decoder.h"

namespace ghirda::sleigh {

DecodeResult Decoder::decode(const std::vector<uint8_t>&, uint64_t) {
  return DecodeResult{"unknown", {}};
}

} // namespace ghirda::sleigh
