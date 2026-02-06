#include "ghirda/core/memory_image.h"

#include <algorithm>
#include <cstring>

namespace ghirda::core {

void MemoryImage::map_segment(uint64_t start, const std::vector<uint8_t>& bytes) {
  segments_.push_back(ImageSegment{start, bytes});
}

void MemoryImage::zero_fill(uint64_t start, uint64_t size) {
  std::vector<uint8_t> zeros(static_cast<size_t>(size), 0);
  segments_.push_back(ImageSegment{start, std::move(zeros)});
}

ImageSegment* MemoryImage::find_segment(uint64_t address) {
  auto it = std::find_if(segments_.begin(), segments_.end(), [address](const ImageSegment& seg) {
    return address >= seg.start && address < seg.start + seg.data.size();
  });
  if (it == segments_.end()) {
    return nullptr;
  }
  return &(*it);
}

const ImageSegment* MemoryImage::find_segment(uint64_t address) const {
  auto it = std::find_if(segments_.begin(), segments_.end(), [address](const ImageSegment& seg) {
    return address >= seg.start && address < seg.start + seg.data.size();
  });
  if (it == segments_.end()) {
    return nullptr;
  }
  return &(*it);
}

bool MemoryImage::read_u32(uint64_t address, uint32_t* value) const {
  const ImageSegment* seg = find_segment(address);
  if (!seg || address + sizeof(uint32_t) > seg->start + seg->data.size()) {
    return false;
  }
  size_t offset = static_cast<size_t>(address - seg->start);
  std::memcpy(value, seg->data.data() + offset, sizeof(uint32_t));
  return true;
}

bool MemoryImage::read_u64(uint64_t address, uint64_t* value) const {
  const ImageSegment* seg = find_segment(address);
  if (!seg || address + sizeof(uint64_t) > seg->start + seg->data.size()) {
    return false;
  }
  size_t offset = static_cast<size_t>(address - seg->start);
  std::memcpy(value, seg->data.data() + offset, sizeof(uint64_t));
  return true;
}

bool MemoryImage::write_u32(uint64_t address, uint32_t value) {
  ImageSegment* seg = find_segment(address);
  if (!seg || address + sizeof(uint32_t) > seg->start + seg->data.size()) {
    return false;
  }
  size_t offset = static_cast<size_t>(address - seg->start);
  std::memcpy(seg->data.data() + offset, &value, sizeof(uint32_t));
  return true;
}

bool MemoryImage::write_u64(uint64_t address, uint64_t value) {
  ImageSegment* seg = find_segment(address);
  if (!seg || address + sizeof(uint64_t) > seg->start + seg->data.size()) {
    return false;
  }
  size_t offset = static_cast<size_t>(address - seg->start);
  std::memcpy(seg->data.data() + offset, &value, sizeof(uint64_t));
  return true;
}

const std::vector<ImageSegment>& MemoryImage::segments() const { return segments_; }

} // namespace ghirda::core
