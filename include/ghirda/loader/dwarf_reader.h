#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "ghirda/core/debug_info.h"

namespace ghirda::loader {

struct DwarfSection {
  const std::vector<uint8_t>* data = nullptr;
};

struct DwarfSections {
  DwarfSection debug_info;
  DwarfSection debug_abbrev;
  DwarfSection debug_line;
  DwarfSection debug_str;
};

class DwarfReader {
public:
  explicit DwarfReader(DwarfSections sections);
  bool parse(ghirda::core::DebugInfo* out, std::string* error);

private:
  struct AbbrevAttr {
    uint32_t name = 0;
    uint32_t form = 0;
  };

  struct AbbrevEntry {
    uint32_t code = 0;
    uint32_t tag = 0;
    bool has_children = false;
    std::vector<AbbrevAttr> attributes;
  };

  struct LineFile {
    std::string name;
    uint32_t dir_index = 0;
  };

  struct LineHeader {
    uint16_t version = 0;
    uint8_t min_inst_length = 1;
    uint8_t max_ops_per_inst = 1;
    uint8_t default_is_stmt = 0;
    int8_t line_base = 0;
    uint8_t line_range = 0;
    uint8_t opcode_base = 0;
    std::vector<uint8_t> standard_opcode_lengths;
    std::vector<std::string> include_dirs;
    std::vector<LineFile> files;
  };

  struct Cursor {
    const std::vector<uint8_t>* data = nullptr;
    size_t offset = 0;

    bool can_read(size_t count) const;
    bool read_u8(uint8_t* value);
    bool read_u16(uint16_t* value);
    bool read_u32(uint32_t* value);
    bool read_u64(uint64_t* value);
    bool read_s64(int64_t* value);
    bool read_uleb(uint64_t* value);
    bool read_sleb(int64_t* value);
    bool read_cstring(std::string* out);
    bool skip(size_t count);
  };

  bool parse_unit(Cursor& cursor, ghirda::core::DebugInfo* out, std::string* error);
  bool parse_abbrev_table(uint64_t offset, std::unordered_map<uint32_t, AbbrevEntry>* table,
                          std::string* error);
  bool parse_die_tree(Cursor& cursor, const std::unordered_map<uint32_t, AbbrevEntry>& abbrev,
                      uint8_t address_size, uint64_t unit_offset, ghirda::core::DebugInfo* out,
                      std::string* error);
  bool parse_line_program(uint64_t offset, ghirda::core::DebugInfo* out, std::string* error);

  bool read_form(Cursor& cursor, uint32_t form, uint8_t address_size, uint64_t unit_offset,
                 uint64_t* uvalue, int64_t* svalue, std::string* str_value);

  std::string read_str(uint64_t offset) const;

  DwarfSections sections_{};
};

} // namespace ghirda::loader
