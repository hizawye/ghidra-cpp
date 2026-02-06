#pragma once

#include <cstdint>
#include <vector>

namespace ghirda::core {

struct ImageSegment {
  uint64_t start = 0;
  std::vector<uint8_t> data;
};

class MemoryImage {
public:
  void map_segment(uint64_t start, const std::vector<uint8_t>& bytes);
  void zero_fill(uint64_t start, uint64_t size);

  bool read_u32(uint64_t address, uint32_t* value) const;
  bool read_u64(uint64_t address, uint64_t* value) const;
  bool write_u32(uint64_t address, uint32_t value);
  bool write_u64(uint64_t address, uint64_t value);

  const std::vector<ImageSegment>& segments() const;

private:
  ImageSegment* find_segment(uint64_t address);
  const ImageSegment* find_segment(uint64_t address) const;
  std::vector<ImageSegment> segments_{};
};

} // namespace ghirda::core
