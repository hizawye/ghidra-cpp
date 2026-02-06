#include "ghirda/loader/dwarf_reader.h"

#include <algorithm>

namespace ghirda::loader {
namespace {

constexpr uint32_t kDwarfTagCompileUnit = 0x11;
constexpr uint32_t kDwarfTagSubprogram = 0x2e;
constexpr uint32_t kDwarfTagBaseType = 0x24;
constexpr uint32_t kDwarfTagPointerType = 0x0f;
constexpr uint32_t kDwarfTagStructureType = 0x13;
constexpr uint32_t kDwarfTagArrayType = 0x01;
constexpr uint32_t kDwarfTagTypedef = 0x16;
constexpr uint32_t kDwarfTagUnionType = 0x17;
constexpr uint32_t kDwarfTagConstType = 0x26;
constexpr uint32_t kDwarfTagVolatileType = 0x35;
constexpr uint32_t kDwarfTagEnumerationType = 0x04;
constexpr uint32_t kDwarfTagSubroutineType = 0x15;
constexpr uint32_t kDwarfTagMember = 0x0d;
constexpr uint32_t kDwarfTagSubrangeType = 0x21;

constexpr uint32_t kDwarfAtName = 0x03;
constexpr uint32_t kDwarfAtLowPc = 0x11;
constexpr uint32_t kDwarfAtHighPc = 0x12;
constexpr uint32_t kDwarfAtByteSize = 0x0b;
constexpr uint32_t kDwarfAtStmtList = 0x10;
constexpr uint32_t kDwarfAtType = 0x49;
constexpr uint32_t kDwarfAtDataMemberLocation = 0x38;
constexpr uint32_t kDwarfAtUpperBound = 0x2f;
constexpr uint32_t kDwarfAtLowerBound = 0x22;
constexpr uint32_t kDwarfAtCount = 0x37;
constexpr uint32_t kDwarfAtBitSize = 0x0d;
constexpr uint32_t kDwarfAtBitOffset = 0x0c;
constexpr uint32_t kDwarfAtDataBitOffset = 0x6b;
constexpr uint32_t kDwarfAtAlignment = 0x88;

constexpr uint32_t kDwarfFormAddr = 0x01;
constexpr uint32_t kDwarfFormData1 = 0x0b;
constexpr uint32_t kDwarfFormData2 = 0x05;
constexpr uint32_t kDwarfFormData4 = 0x06;
constexpr uint32_t kDwarfFormData8 = 0x07;
constexpr uint32_t kDwarfFormSdata = 0x0d;
constexpr uint32_t kDwarfFormUdata = 0x0f;
constexpr uint32_t kDwarfFormString = 0x08;
constexpr uint32_t kDwarfFormStrp = 0x0e;
constexpr uint32_t kDwarfFormSecOffset = 0x17;
constexpr uint32_t kDwarfFormFlag = 0x0c;
constexpr uint32_t kDwarfFormRef1 = 0x11;
constexpr uint32_t kDwarfFormRef2 = 0x12;
constexpr uint32_t kDwarfFormRef4 = 0x13;
constexpr uint32_t kDwarfFormRef8 = 0x14;
constexpr uint32_t kDwarfFormRefUdata = 0x15;
constexpr uint32_t kDwarfFormRefAddr = 0x10;
constexpr uint32_t kDwarfFormFlagPresent = 0x19;
constexpr uint32_t kDwarfFormExprloc = 0x18;
constexpr uint32_t kDwarfFormBlock1 = 0x0a;
constexpr uint32_t kDwarfFormBlock2 = 0x03;
constexpr uint32_t kDwarfFormBlock4 = 0x04;
constexpr uint32_t kDwarfFormBlock = 0x09;

constexpr uint8_t kLineOpCopy = 1;
constexpr uint8_t kLineOpAdvancePc = 2;
constexpr uint8_t kLineOpAdvanceLine = 3;
constexpr uint8_t kLineOpSetFile = 4;
constexpr uint8_t kLineOpSetColumn = 5;
constexpr uint8_t kLineOpNegStmt = 6;
constexpr uint8_t kLineOpSetBasicBlock = 7;
constexpr uint8_t kLineOpConstAddPc = 8;
constexpr uint8_t kLineOpFixedAdvancePc = 9;
constexpr uint8_t kLineOpSetPrologueEnd = 10;
constexpr uint8_t kLineOpSetEpilogueBegin = 11;
constexpr uint8_t kLineOpSetIsa = 12;

std::string join_path(const std::string& dir, const std::string& file) {
  if (dir.empty()) {
    return file;
  }
  return dir + "/" + file;
}

bool is_high_pc_offset_form(uint32_t form) {
  return form != kDwarfFormAddr;
}

} // namespace

DwarfReader::DwarfReader(DwarfSections sections) : sections_(sections) {}

bool DwarfReader::Cursor::can_read(size_t count) const {
  return data && offset + count <= data->size();
}

bool DwarfReader::Cursor::read_u8(uint8_t* value) {
  if (!can_read(1)) {
    return false;
  }
  *value = (*data)[offset++];
  return true;
}

bool DwarfReader::Cursor::read_u16(uint16_t* value) {
  if (!can_read(2)) {
    return false;
  }
  *value = static_cast<uint16_t>((*data)[offset]) |
           (static_cast<uint16_t>((*data)[offset + 1]) << 8);
  offset += 2;
  return true;
}

bool DwarfReader::Cursor::read_u32(uint32_t* value) {
  if (!can_read(4)) {
    return false;
  }
  *value = static_cast<uint32_t>((*data)[offset]) |
           (static_cast<uint32_t>((*data)[offset + 1]) << 8) |
           (static_cast<uint32_t>((*data)[offset + 2]) << 16) |
           (static_cast<uint32_t>((*data)[offset + 3]) << 24);
  offset += 4;
  return true;
}

bool DwarfReader::Cursor::read_u64(uint64_t* value) {
  if (!can_read(8)) {
    return false;
  }
  uint64_t result = 0;
  for (int i = 0; i < 8; ++i) {
    result |= static_cast<uint64_t>((*data)[offset + i]) << (8 * i);
  }
  *value = result;
  offset += 8;
  return true;
}

bool DwarfReader::Cursor::read_s64(int64_t* value) {
  uint64_t tmp = 0;
  if (!read_u64(&tmp)) {
    return false;
  }
  *value = static_cast<int64_t>(tmp);
  return true;
}

bool DwarfReader::Cursor::read_uleb(uint64_t* value) {
  uint64_t result = 0;
  uint8_t shift = 0;
  while (true) {
    uint8_t byte = 0;
    if (!read_u8(&byte)) {
      return false;
    }
    result |= static_cast<uint64_t>(byte & 0x7f) << shift;
    if ((byte & 0x80) == 0) {
      break;
    }
    shift += 7;
  }
  *value = result;
  return true;
}

bool DwarfReader::Cursor::read_sleb(int64_t* value) {
  int64_t result = 0;
  uint8_t shift = 0;
  uint8_t byte = 0;
  while (true) {
    if (!read_u8(&byte)) {
      return false;
    }
    result |= static_cast<int64_t>(byte & 0x7f) << shift;
    shift += 7;
    if ((byte & 0x80) == 0) {
      break;
    }
  }
  if (shift < 64 && (byte & 0x40)) {
    result |= - (1LL << shift);
  }
  *value = result;
  return true;
}

bool DwarfReader::Cursor::read_cstring(std::string* out) {
  if (!data) {
    return false;
  }
  size_t start = offset;
  while (offset < data->size() && (*data)[offset] != 0) {
    ++offset;
  }
  if (offset >= data->size()) {
    return false;
  }
  *out = std::string(reinterpret_cast<const char*>(data->data() + start), offset - start);
  ++offset;
  return true;
}

bool DwarfReader::Cursor::skip(size_t count) {
  if (!can_read(count)) {
    return false;
  }
  offset += count;
  return true;
}

std::string DwarfReader::read_str(uint64_t offset) const {
  if (!sections_.debug_str.data || offset >= sections_.debug_str.data->size()) {
    return {};
  }
  const auto* data = sections_.debug_str.data;
  const char* start = reinterpret_cast<const char*>(data->data() + offset);
  return std::string(start);
}

bool DwarfReader::parse(ghirda::core::DebugInfo* out, std::string* error) {
  if (!sections_.debug_info.data || !sections_.debug_abbrev.data) {
    if (error) {
      *error = "missing debug sections";
    }
    return false;
  }

  Cursor cursor{sections_.debug_info.data, 0};
  while (cursor.offset < sections_.debug_info.data->size()) {
    if (!parse_unit(cursor, out, error)) {
      return false;
    }
  }

  return true;
}

bool DwarfReader::parse_unit(Cursor& cursor, ghirda::core::DebugInfo* out, std::string* error) {
  uint32_t unit_length = 0;
  if (!cursor.read_u32(&unit_length)) {
    return false;
  }
  uint64_t unit_start = cursor.offset - 4;
  if (unit_length == 0) {
    return true;
  }
  if (unit_length == 0xffffffffu) {
    if (error) {
      *error = "DWARF64 not supported";
    }
    return false;
  }

  size_t unit_end = cursor.offset + unit_length;
  uint16_t version = 0;
  if (!cursor.read_u16(&version)) {
    return false;
  }
  if (version < 4) {
    if (error) {
      *error = "DWARF version < 4 not supported";
    }
    return false;
  }

  uint32_t abbrev_offset = 0;
  if (!cursor.read_u32(&abbrev_offset)) {
    return false;
  }

  uint8_t address_size = 0;
  if (!cursor.read_u8(&address_size)) {
    return false;
  }

  std::unordered_map<uint32_t, AbbrevEntry> abbrev;
  if (!parse_abbrev_table(abbrev_offset, &abbrev, error)) {
    return false;
  }

  if (!parse_die_tree(cursor, abbrev, address_size, unit_start, out, error)) {
    return false;
  }

  cursor.offset = unit_end;
  return true;
}

bool DwarfReader::parse_abbrev_table(uint64_t offset, std::unordered_map<uint32_t, AbbrevEntry>* table,
                                     std::string* error) {
  Cursor cursor{sections_.debug_abbrev.data, static_cast<size_t>(offset)};
  if (!cursor.data || cursor.offset >= cursor.data->size()) {
    if (error) {
      *error = "invalid abbrev offset";
    }
    return false;
  }

  while (cursor.offset < cursor.data->size()) {
    uint64_t code = 0;
    if (!cursor.read_uleb(&code)) {
      return false;
    }
    if (code == 0) {
      break;
    }

    uint64_t tag = 0;
    if (!cursor.read_uleb(&tag)) {
      return false;
    }

    uint8_t has_children = 0;
    if (!cursor.read_u8(&has_children)) {
      return false;
    }

    AbbrevEntry entry;
    entry.code = static_cast<uint32_t>(code);
    entry.tag = static_cast<uint32_t>(tag);
    entry.has_children = (has_children != 0);

    while (true) {
      uint64_t attr_name = 0;
      uint64_t attr_form = 0;
      if (!cursor.read_uleb(&attr_name)) {
        return false;
      }
      if (!cursor.read_uleb(&attr_form)) {
        return false;
      }
      if (attr_name == 0 && attr_form == 0) {
        break;
      }
      entry.attributes.push_back(AbbrevAttr{static_cast<uint32_t>(attr_name), static_cast<uint32_t>(attr_form)});
    }

    (*table)[entry.code] = entry;
  }

  return true;
}

bool DwarfReader::read_form(Cursor& cursor, uint32_t form, uint8_t address_size, uint64_t unit_offset,
                            uint64_t* uvalue, int64_t* svalue, std::string* str_value) {
  if (uvalue) {
    *uvalue = 0;
  }
  if (svalue) {
    *svalue = 0;
  }
  if (str_value) {
    str_value->clear();
  }

  switch (form) {
    case kDwarfFormAddr: {
      uint64_t value = 0;
      if (address_size == 8) {
        if (!cursor.read_u64(&value)) {
          return false;
        }
      } else {
        uint32_t value32 = 0;
        if (!cursor.read_u32(&value32)) {
          return false;
        }
        value = value32;
      }
      if (uvalue) {
        *uvalue = value;
      }
      return true;
    }
    case kDwarfFormData1: {
      uint8_t value = 0;
      if (!cursor.read_u8(&value)) {
        return false;
      }
      if (uvalue) {
        *uvalue = value;
      }
      return true;
    }
    case kDwarfFormData2: {
      uint16_t value = 0;
      if (!cursor.read_u16(&value)) {
        return false;
      }
      if (uvalue) {
        *uvalue = value;
      }
      return true;
    }
    case kDwarfFormData4: {
      uint32_t value = 0;
      if (!cursor.read_u32(&value)) {
        return false;
      }
      if (uvalue) {
        *uvalue = value;
      }
      return true;
    }
    case kDwarfFormData8: {
      uint64_t value = 0;
      if (!cursor.read_u64(&value)) {
        return false;
      }
      if (uvalue) {
        *uvalue = value;
      }
      return true;
    }
    case kDwarfFormSdata: {
      int64_t value = 0;
      if (!cursor.read_sleb(&value)) {
        return false;
      }
      if (svalue) {
        *svalue = value;
      }
      return true;
    }
    case kDwarfFormUdata: {
      uint64_t value = 0;
      if (!cursor.read_uleb(&value)) {
        return false;
      }
      if (uvalue) {
        *uvalue = value;
      }
      return true;
    }
    case kDwarfFormString: {
      if (!str_value) {
        std::string tmp;
        return cursor.read_cstring(&tmp);
      }
      return cursor.read_cstring(str_value);
    }
    case kDwarfFormStrp: {
      uint32_t offset = 0;
      if (!cursor.read_u32(&offset)) {
        return false;
      }
      if (str_value) {
        *str_value = read_str(offset);
      }
      return true;
    }
    case kDwarfFormSecOffset: {
      uint32_t value = 0;
      if (!cursor.read_u32(&value)) {
        return false;
      }
      if (uvalue) {
        *uvalue = value;
      }
      return true;
    }
    case kDwarfFormFlag: {
      uint8_t value = 0;
      if (!cursor.read_u8(&value)) {
        return false;
      }
      if (uvalue) {
        *uvalue = value;
      }
      return true;
    }
    case kDwarfFormFlagPresent: {
      if (uvalue) {
        *uvalue = 1;
      }
      return true;
    }
    case kDwarfFormRef1: {
      uint8_t value = 0;
      if (!cursor.read_u8(&value)) {
        return false;
      }
      if (uvalue) {
        *uvalue = unit_offset + value;
      }
      return true;
    }
    case kDwarfFormRef2: {
      uint16_t value = 0;
      if (!cursor.read_u16(&value)) {
        return false;
      }
      if (uvalue) {
        *uvalue = unit_offset + value;
      }
      return true;
    }
    case kDwarfFormRef4: {
      uint32_t value = 0;
      if (!cursor.read_u32(&value)) {
        return false;
      }
      if (uvalue) {
        *uvalue = unit_offset + value;
      }
      return true;
    }
    case kDwarfFormRef8: {
      uint64_t value = 0;
      if (!cursor.read_u64(&value)) {
        return false;
      }
      if (uvalue) {
        *uvalue = unit_offset + value;
      }
      return true;
    }
    case kDwarfFormRefUdata: {
      uint64_t value = 0;
      if (!cursor.read_uleb(&value)) {
        return false;
      }
      if (uvalue) {
        *uvalue = unit_offset + value;
      }
      return true;
    }
    case kDwarfFormRefAddr: {
      uint64_t value = 0;
      if (address_size == 8) {
        if (!cursor.read_u64(&value)) {
          return false;
        }
      } else {
        uint32_t value32 = 0;
        if (!cursor.read_u32(&value32)) {
          return false;
        }
        value = value32;
      }
      if (uvalue) {
        *uvalue = value;
      }
      return true;
    }
    case kDwarfFormExprloc: {
      uint64_t length = 0;
      if (!cursor.read_uleb(&length)) {
        return false;
      }
      return cursor.skip(static_cast<size_t>(length));
    }
    case kDwarfFormBlock1: {
      uint8_t length = 0;
      if (!cursor.read_u8(&length)) {
        return false;
      }
      return cursor.skip(length);
    }
    case kDwarfFormBlock2: {
      uint16_t length = 0;
      if (!cursor.read_u16(&length)) {
        return false;
      }
      return cursor.skip(length);
    }
    case kDwarfFormBlock4: {
      uint32_t length = 0;
      if (!cursor.read_u32(&length)) {
        return false;
      }
      return cursor.skip(length);
    }
    case kDwarfFormBlock: {
      uint64_t length = 0;
      if (!cursor.read_uleb(&length)) {
        return false;
      }
      return cursor.skip(static_cast<size_t>(length));
    }
    default:
      return false;
  }
}

bool DwarfReader::parse_die_tree(Cursor& cursor, const std::unordered_map<uint32_t, AbbrevEntry>& abbrev,
                                 uint8_t address_size, uint64_t unit_offset, ghirda::core::DebugInfo* out,
                                 std::string* error) {
  std::vector<bool> has_children_stack;
  std::vector<int> type_stack;

  while (cursor.offset < cursor.data->size()) {
    uint64_t code = 0;
    if (!cursor.read_uleb(&code)) {
      return false;
    }
    if (code == 0) {
      if (has_children_stack.empty()) {
        return true;
      }
      has_children_stack.pop_back();
      if (!type_stack.empty()) {
        type_stack.pop_back();
      }
      continue;
    }

    auto it = abbrev.find(static_cast<uint32_t>(code));
    if (it == abbrev.end()) {
      if (error) {
        *error = "unknown abbrev code";
      }
      return false;
    }

    const AbbrevEntry& entry = it->second;
    uint64_t die_offset = cursor.offset;
    uint64_t low_pc = 0;
    uint64_t high_pc = 0;
    uint64_t stmt_list = 0;
    uint64_t byte_size = 0;
    uint64_t type_ref = 0;
    uint64_t member_location = 0;
    uint64_t upper_bound = 0;
    uint64_t lower_bound = 0;
    uint64_t count = 0;
    uint64_t bit_size = 0;
    int64_t bit_offset = -1;
    int64_t data_bit_offset = -1;
    uint64_t alignment = 0;
    std::string name;
    uint32_t high_pc_form = 0;

    for (const auto& attr : entry.attributes) {
      uint64_t uvalue = 0;
      int64_t svalue = 0;
      std::string str_value;
      if (!read_form(cursor, attr.form, address_size, unit_offset, &uvalue, &svalue, &str_value)) {
        return false;
      }

      switch (attr.name) {
        case kDwarfAtName:
          name = str_value;
          break;
        case kDwarfAtLowPc:
          low_pc = uvalue;
          break;
        case kDwarfAtHighPc:
          high_pc = uvalue;
          high_pc_form = attr.form;
          break;
        case kDwarfAtStmtList:
          stmt_list = uvalue;
          break;
        case kDwarfAtByteSize:
          byte_size = uvalue;
          break;
        case kDwarfAtType:
          type_ref = uvalue;
          break;
        case kDwarfAtDataMemberLocation:
          member_location = uvalue;
          break;
        case kDwarfAtUpperBound:
          upper_bound = uvalue;
          break;
        case kDwarfAtLowerBound:
          lower_bound = uvalue;
          break;
        case kDwarfAtCount:
          count = uvalue;
          break;
        case kDwarfAtBitSize:
          bit_size = uvalue;
          break;
        case kDwarfAtBitOffset:
          bit_offset = static_cast<int64_t>(uvalue);
          break;
        case kDwarfAtDataBitOffset:
          data_bit_offset = static_cast<int64_t>(uvalue);
          break;
        case kDwarfAtAlignment:
          alignment = uvalue;
          break;
        default:
          break;
      }
    }

    if (high_pc != 0 && low_pc != 0 && is_high_pc_offset_form(high_pc_form)) {
      high_pc = low_pc + high_pc;
    }

    if (entry.tag == kDwarfTagCompileUnit && stmt_list != 0) {
      parse_line_program(stmt_list, out, error);
    }

    if (entry.tag == kDwarfTagSubprogram && !name.empty()) {
      ghirda::core::DebugFunction func{};
      func.name = name;
      func.low_pc = low_pc;
      func.high_pc = high_pc;
      func.return_type_ref = type_ref;
      out->functions.push_back(func);
    }

    if (entry.tag == kDwarfTagMember) {
      if (!type_stack.empty() && type_stack.back() >= 0) {
        ghirda::core::DebugMember member{};
        member.name = name;
        member.type_ref = type_ref;
        member.offset = member_location;
        member.bit_size = static_cast<uint32_t>(bit_size);
        member.bit_offset = static_cast<int32_t>(data_bit_offset >= 0 ? data_bit_offset : bit_offset);
        member.alignment = static_cast<uint32_t>(alignment);
        out->types[static_cast<size_t>(type_stack.back())].members.push_back(member);
      }
    }

    if (entry.tag == kDwarfTagSubrangeType) {
      if (!type_stack.empty() && type_stack.back() >= 0) {
        auto& parent = out->types[static_cast<size_t>(type_stack.back())];
        if (parent.kind == ghirda::core::DebugTypeKind::Array) {
          uint64_t range_count = count;
          if (range_count == 0 && upper_bound >= lower_bound) {
            range_count = upper_bound - lower_bound + 1;
          }
          if (range_count != 0) {
            parent.array_count = range_count;
          }
        }
      }
    }

    bool pushed_type = false;
    if (entry.tag == kDwarfTagBaseType || entry.tag == kDwarfTagPointerType ||
        entry.tag == kDwarfTagStructureType || entry.tag == kDwarfTagArrayType ||
        entry.tag == kDwarfTagTypedef || entry.tag == kDwarfTagUnionType ||
        entry.tag == kDwarfTagConstType || entry.tag == kDwarfTagVolatileType ||
        entry.tag == kDwarfTagEnumerationType || entry.tag == kDwarfTagSubroutineType) {
      ghirda::core::DebugType type{};
      type.name = name;
      type.size = static_cast<uint32_t>(byte_size);
      type.type_ref = type_ref;
      type.die_offset = die_offset;
      switch (entry.tag) {
        case kDwarfTagBaseType:
          type.kind = ghirda::core::DebugTypeKind::Base;
          break;
        case kDwarfTagPointerType:
          type.kind = ghirda::core::DebugTypeKind::Pointer;
          break;
        case kDwarfTagStructureType:
          type.kind = ghirda::core::DebugTypeKind::Struct;
          break;
        case kDwarfTagArrayType:
          type.kind = ghirda::core::DebugTypeKind::Array;
          break;
        case kDwarfTagTypedef:
          type.kind = ghirda::core::DebugTypeKind::Typedef;
          break;
        case kDwarfTagUnionType:
          type.kind = ghirda::core::DebugTypeKind::Union;
          break;
        case kDwarfTagConstType:
          type.kind = ghirda::core::DebugTypeKind::Const;
          break;
        case kDwarfTagVolatileType:
          type.kind = ghirda::core::DebugTypeKind::Volatile;
          break;
        case kDwarfTagEnumerationType:
          type.kind = ghirda::core::DebugTypeKind::Enumeration;
          break;
        case kDwarfTagSubroutineType:
          type.kind = ghirda::core::DebugTypeKind::Subroutine;
          break;
        default:
          type.kind = ghirda::core::DebugTypeKind::Unknown;
          break;
      }
      if (!type.name.empty()) {
        out->types.push_back(type);
        if (entry.has_children) {
          type_stack.push_back(static_cast<int>(out->types.size() - 1));
          pushed_type = true;
        }
      }
    }

    if (entry.has_children) {
      has_children_stack.push_back(true);
      if (!pushed_type) {
        type_stack.push_back(-1);
      }
    }
  }

  return true;
}

bool DwarfReader::parse_line_program(uint64_t offset, ghirda::core::DebugInfo* out, std::string* error) {
  if (!sections_.debug_line.data) {
    return false;
  }

  Cursor cursor{sections_.debug_line.data, static_cast<size_t>(offset)};
  uint32_t unit_length = 0;
  if (!cursor.read_u32(&unit_length)) {
    return false;
  }
  if (unit_length == 0 || unit_length == 0xffffffffu) {
    return false;
  }
  size_t unit_end = cursor.offset + unit_length;

  LineHeader header{};
  if (!cursor.read_u16(&header.version)) {
    return false;
  }
  if (header.version < 4) {
    if (error) {
      *error = "DWARF line version < 4 not supported";
    }
    return false;
  }

  uint32_t header_length = 0;
  if (!cursor.read_u32(&header_length)) {
    return false;
  }
  size_t header_end = cursor.offset + header_length;

  if (!cursor.read_u8(&header.min_inst_length)) {
    return false;
  }
  if (!cursor.read_u8(&header.max_ops_per_inst)) {
    return false;
  }
  if (!cursor.read_u8(&header.default_is_stmt)) {
    return false;
  }
  if (!cursor.read_u8(reinterpret_cast<uint8_t*>(&header.line_base))) {
    return false;
  }
  if (!cursor.read_u8(&header.line_range)) {
    return false;
  }
  if (!cursor.read_u8(&header.opcode_base)) {
    return false;
  }

  header.standard_opcode_lengths.resize(header.opcode_base > 0 ? header.opcode_base - 1 : 0);
  for (size_t i = 0; i < header.standard_opcode_lengths.size(); ++i) {
    if (!cursor.read_u8(&header.standard_opcode_lengths[i])) {
      return false;
    }
  }

  while (cursor.offset < header_end) {
    std::string dir;
    if (!cursor.read_cstring(&dir)) {
      return false;
    }
    if (dir.empty()) {
      break;
    }
    header.include_dirs.push_back(dir);
  }

  while (cursor.offset < header_end) {
    std::string name;
    if (!cursor.read_cstring(&name)) {
      return false;
    }
    if (name.empty()) {
      break;
    }
    uint64_t dir_index = 0;
    uint64_t mod_time = 0;
    uint64_t length = 0;
    if (!cursor.read_uleb(&dir_index) || !cursor.read_uleb(&mod_time) || !cursor.read_uleb(&length)) {
      return false;
    }
    LineFile file;
    file.name = name;
    file.dir_index = static_cast<uint32_t>(dir_index);
    header.files.push_back(file);
  }

  uint64_t address = 0;
  uint32_t line = 1;
  uint32_t file = 1;
  bool is_stmt = header.default_is_stmt != 0;

  while (cursor.offset < unit_end) {
    uint8_t opcode = 0;
    if (!cursor.read_u8(&opcode)) {
      return false;
    }

    if (opcode == 0) {
      uint64_t ext_len = 0;
      if (!cursor.read_uleb(&ext_len)) {
        return false;
      }
      uint8_t sub = 0;
      if (!cursor.read_u8(&sub)) {
        return false;
      }
      if (sub == 1) {
        address = 0;
        line = 1;
        file = 1;
        is_stmt = header.default_is_stmt != 0;
      } else {
        if (!cursor.skip(ext_len - 1)) {
          return false;
        }
      }
      continue;
    }

    if (opcode < header.opcode_base) {
      switch (opcode) {
        case kLineOpCopy: {
          if (file > 0 && file <= header.files.size()) {
            const auto& f = header.files[file - 1];
            std::string dir;
            if (f.dir_index > 0 && f.dir_index <= header.include_dirs.size()) {
              dir = header.include_dirs[f.dir_index - 1];
            }
            ghirda::core::DebugLineEntry entry{};
            entry.address = address;
            entry.line = line;
            entry.file = join_path(dir, f.name);
            out->lines.push_back(entry);
          }
          break;
        }
        case kLineOpAdvancePc: {
          uint64_t advance = 0;
          if (!cursor.read_uleb(&advance)) {
            return false;
          }
          address += advance * header.min_inst_length;
          break;
        }
        case kLineOpAdvanceLine: {
          int64_t delta = 0;
          if (!cursor.read_sleb(&delta)) {
            return false;
          }
          line = static_cast<uint32_t>(static_cast<int64_t>(line) + delta);
          break;
        }
        case kLineOpSetFile: {
          uint64_t value = 0;
          if (!cursor.read_uleb(&value)) {
            return false;
          }
          file = static_cast<uint32_t>(value);
          break;
        }
        case kLineOpSetColumn: {
          uint64_t value = 0;
          if (!cursor.read_uleb(&value)) {
            return false;
          }
          break;
        }
        case kLineOpNegStmt:
          is_stmt = !is_stmt;
          break;
        case kLineOpSetBasicBlock:
        case kLineOpSetPrologueEnd:
        case kLineOpSetEpilogueBegin:
          break;
        case kLineOpConstAddPc: {
          uint8_t adjusted = 255 - header.opcode_base;
          uint64_t advance = (adjusted / header.line_range) * header.min_inst_length;
          address += advance;
          break;
        }
        case kLineOpFixedAdvancePc: {
          uint16_t advance = 0;
          if (!cursor.read_u16(&advance)) {
            return false;
          }
          address += advance;
          break;
        }
        case kLineOpSetIsa: {
          uint64_t value = 0;
          if (!cursor.read_uleb(&value)) {
            return false;
          }
          break;
        }
        default: {
          uint8_t arg_count = 0;
          if (opcode > 0 && opcode - 1 < header.standard_opcode_lengths.size()) {
            arg_count = header.standard_opcode_lengths[opcode - 1];
          }
          for (uint8_t i = 0; i < arg_count; ++i) {
            uint64_t dummy = 0;
            if (!cursor.read_uleb(&dummy)) {
              return false;
            }
          }
          break;
        }
      }
      continue;
    }

    uint8_t adjusted = opcode - header.opcode_base;
    uint64_t advance_addr = (adjusted / header.line_range) * header.min_inst_length;
    int64_t advance_line = header.line_base + static_cast<int8_t>(adjusted % header.line_range);
    address += advance_addr;
    line = static_cast<uint32_t>(static_cast<int64_t>(line) + advance_line);
    if (file > 0 && file <= header.files.size()) {
      const auto& f = header.files[file - 1];
      std::string dir;
      if (f.dir_index > 0 && f.dir_index <= header.include_dirs.size()) {
        dir = header.include_dirs[f.dir_index - 1];
      }
      ghirda::core::DebugLineEntry entry{};
      entry.address = address;
      entry.line = line;
      entry.file = join_path(dir, f.name);
      out->lines.push_back(entry);
    }
  }

  return true;
}

} // namespace ghirda::loader
