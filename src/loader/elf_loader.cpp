#include "ghirda/loader/elf_loader.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ghirda/core/address_space.h"
#include "ghirda/core/memory_map.h"
#include "ghirda/core/relocation.h"
#include "ghirda/core/symbol.h"
#include "ghirda/core/type_system.h"
#include "ghirda/loader/dwarf_reader.h"

namespace ghirda::loader {
namespace {

constexpr std::array<uint8_t, 4> kElfMagic{0x7f, 'E', 'L', 'F'};
constexpr uint16_t kElfClass64 = 2;
constexpr uint16_t kElfDataLittle = 1;
constexpr uint32_t kElfTypeExecutable = 2;
constexpr uint32_t kElfTypeShared = 3;
constexpr uint32_t kElfPtLoad = 1;
constexpr uint32_t kElfShtSymtab = 2;
constexpr uint32_t kElfShtStrtab = 3;
constexpr uint32_t kElfShtRela = 4;
constexpr uint32_t kElfShtRel = 9;
constexpr uint32_t kElfShtDynsym = 11;

constexpr uint8_t kElfSttNotype = 0;
constexpr uint8_t kElfSttObject = 1;
constexpr uint8_t kElfSttFunc = 2;
constexpr uint8_t kElfSttSection = 3;

constexpr uint32_t kRelaX86_64_64 = 1;
constexpr uint32_t kRelaX86_64_PC32 = 2;
constexpr uint32_t kRelaX86_64_32 = 10;
constexpr uint32_t kRelaX86_64_32S = 11;
constexpr uint32_t kRelaX86_64_GlobDat = 6;
constexpr uint32_t kRelaX86_64_JumpSlot = 7;
constexpr uint32_t kRelaX86_64_Relative = 8;

struct Elf64Header {
  uint8_t ident[16];
  uint16_t type;
  uint16_t machine;
  uint32_t version;
  uint64_t entry;
  uint64_t phoff;
  uint64_t shoff;
  uint32_t flags;
  uint16_t ehsize;
  uint16_t phentsize;
  uint16_t phnum;
  uint16_t shentsize;
  uint16_t shnum;
  uint16_t shstrndx;
};

struct Elf64Phdr {
  uint32_t type;
  uint32_t flags;
  uint64_t offset;
  uint64_t vaddr;
  uint64_t paddr;
  uint64_t filesz;
  uint64_t memsz;
  uint64_t align;
};

struct Elf64Shdr {
  uint32_t name;
  uint32_t type;
  uint64_t flags;
  uint64_t addr;
  uint64_t offset;
  uint64_t size;
  uint32_t link;
  uint32_t info;
  uint64_t addralign;
  uint64_t entsize;
};

struct Elf64Sym {
  uint32_t name;
  uint8_t info;
  uint8_t other;
  uint16_t shndx;
  uint64_t value;
  uint64_t size;
};

struct Elf64Rela {
  uint64_t offset;
  uint64_t info;
  int64_t addend;
};

struct Elf64Rel {
  uint64_t offset;
  uint64_t info;
};

bool read_exact(std::ifstream& in, void* data, size_t size) {
  in.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(size));
  return in.good();
}

bool read_blob(std::ifstream& in, uint64_t offset, uint64_t size, std::vector<uint8_t>* out) {
  out->assign(static_cast<size_t>(size), 0);
  in.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
  if (!in) {
    return false;
  }
  in.read(reinterpret_cast<char*>(out->data()), static_cast<std::streamsize>(size));
  return in.good();
}

std::string read_string(const std::vector<uint8_t>& table, uint32_t offset) {
  if (offset >= table.size()) {
    return {};
  }
  const char* start = reinterpret_cast<const char*>(table.data() + offset);
  return std::string(start);
}

uint8_t symbol_type(uint8_t info) { return static_cast<uint8_t>(info & 0x0f); }

uint32_t reloc_type(uint64_t info) { return static_cast<uint32_t>(info & 0xffffffffu); }

uint32_t reloc_sym_index(uint64_t info) { return static_cast<uint32_t>(info >> 32u); }

ghirda::core::SymbolKind to_symbol_kind(uint8_t type) {
  switch (type) {
    case kElfSttFunc:
      return ghirda::core::SymbolKind::Function;
    case kElfSttObject:
      return ghirda::core::SymbolKind::Data;
    case kElfSttSection:
      return ghirda::core::SymbolKind::Label;
    default:
      return ghirda::core::SymbolKind::Unknown;
  }
}

bool apply_reloc(uint32_t type, uint64_t address, uint64_t symbol_value, int64_t addend,
                 uint64_t load_bias, ghirda::core::MemoryImage* image, std::string* note) {
  if (!image) {
    if (note) {
      *note = "no memory image";
    }
    return false;
  }

  const uint64_t place = address + load_bias;
  switch (type) {
    case kRelaX86_64_64: {
      uint64_t value = symbol_value + static_cast<uint64_t>(addend) + load_bias;
      return image->write_u64(place, value);
    }
    case kRelaX86_64_PC32: {
      uint64_t value = symbol_value + static_cast<uint64_t>(addend) + load_bias;
      uint64_t result = value - place;
      return image->write_u32(place, static_cast<uint32_t>(result));
    }
    case kRelaX86_64_32: {
      uint64_t value = symbol_value + static_cast<uint64_t>(addend) + load_bias;
      return image->write_u32(place, static_cast<uint32_t>(value));
    }
    case kRelaX86_64_32S: {
      int64_t value = static_cast<int64_t>(symbol_value) + addend + static_cast<int64_t>(load_bias);
      return image->write_u32(place, static_cast<uint32_t>(value));
    }
    case kRelaX86_64_GlobDat:
    case kRelaX86_64_JumpSlot: {
      uint64_t value = symbol_value + load_bias;
      return image->write_u64(place, value);
    }
    case kRelaX86_64_Relative: {
      uint64_t value = load_bias + static_cast<uint64_t>(addend);
      return image->write_u64(place, value);
    }
    default:
      if (note) {
        *note = "unsupported relocation";
      }
      return false;
  }
}

} // namespace

bool ElfLoader::load(const std::string& path, ghirda::core::Program* program, std::string* error) {
  if (!program) {
    if (error) {
      *error = "program output is null";
    }
    return false;
  }

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error) {
      *error = "failed to open file";
    }
    return false;
  }

  Elf64Header header{};
  if (!read_exact(in, &header, sizeof(header))) {
    if (error) {
      *error = "failed to read ELF header";
    }
    return false;
  }

  if (!std::equal(std::begin(kElfMagic), std::end(kElfMagic), header.ident)) {
    if (error) {
      *error = "not an ELF file";
    }
    return false;
  }

  if (header.ident[4] != kElfClass64 || header.ident[5] != kElfDataLittle) {
    if (error) {
      *error = "unsupported ELF class or endianness";
    }
    return false;
  }

  if (header.type != kElfTypeExecutable && header.type != kElfTypeShared) {
    if (error) {
      *error = "unsupported ELF type";
    }
    return false;
  }

  if (header.phoff == 0 || header.phnum == 0) {
    if (error) {
      *error = "ELF has no program headers";
    }
    return false;
  }

  if (header.phentsize != sizeof(Elf64Phdr)) {
    if (error) {
      *error = "unexpected program header size";
    }
    return false;
  }

  in.seekg(static_cast<std::streamoff>(header.phoff), std::ios::beg);
  if (!in) {
    if (error) {
      *error = "failed to seek to program headers";
    }
    return false;
  }

  uint64_t min_vaddr = UINT64_MAX;
  uint64_t max_vaddr = 0;
  bool found_load = false;

  for (uint16_t i = 0; i < header.phnum; ++i) {
    Elf64Phdr phdr{};
    if (!read_exact(in, &phdr, sizeof(phdr))) {
      if (error) {
        *error = "failed to read program header";
      }
      return false;
    }

    if (phdr.type != kElfPtLoad || phdr.memsz == 0) {
      continue;
    }

    ghirda::core::MemoryRegion region{};
    region.start = phdr.vaddr;
    region.size = phdr.memsz;
    region.readable = (phdr.flags & 0x4u) != 0;
    region.writable = (phdr.flags & 0x2u) != 0;
    region.executable = (phdr.flags & 0x1u) != 0;
    program->memory_map().add_region(region);

    std::vector<uint8_t> bytes;
    if (!read_blob(in, phdr.offset, phdr.filesz, &bytes)) {
      if (error) {
        *error = "failed to read segment bytes";
      }
      return false;
    }

    program->memory_image().map_segment(phdr.vaddr, bytes);
    if (phdr.memsz > phdr.filesz) {
      program->memory_image().zero_fill(phdr.vaddr + phdr.filesz, phdr.memsz - phdr.filesz);
    }

    min_vaddr = std::min(min_vaddr, phdr.vaddr);
    max_vaddr = std::max(max_vaddr, phdr.vaddr + phdr.memsz);
    found_load = true;
  }

  if (!found_load) {
    if (error) {
      *error = "no loadable segments";
    }
    return false;
  }

  if (min_vaddr < max_vaddr) {
    program->add_address_space(ghirda::core::AddressSpace("ram", min_vaddr, max_vaddr - min_vaddr));
  }

  if (header.type == kElfTypeShared) {
    program->set_load_bias(min_vaddr);
  } else {
    program->set_load_bias(0);
  }

  if (header.shoff == 0 || header.shnum == 0) {
    return true;
  }

  if (header.shentsize != sizeof(Elf64Shdr)) {
    if (error) {
      *error = "unexpected section header size";
    }
    return false;
  }

  std::vector<Elf64Shdr> sections(header.shnum);
  in.seekg(static_cast<std::streamoff>(header.shoff), std::ios::beg);
  if (!in) {
    if (error) {
      *error = "failed to seek to section headers";
    }
    return false;
  }

  for (uint16_t i = 0; i < header.shnum; ++i) {
    if (!read_exact(in, &sections[i], sizeof(Elf64Shdr))) {
      if (error) {
        *error = "failed to read section header";
      }
      return false;
    }
  }

  if (header.shstrndx >= sections.size()) {
    if (error) {
      *error = "invalid section string table index";
    }
    return false;
  }

  std::vector<uint8_t> shstrtab;
  if (!read_blob(in, sections[header.shstrndx].offset, sections[header.shstrndx].size, &shstrtab)) {
    if (error) {
      *error = "failed to read section string table";
    }
    return false;
  }

  std::vector<std::vector<uint8_t>> string_tables(sections.size());
  std::vector<std::vector<Elf64Sym>> symbol_tables(sections.size());

  for (size_t i = 0; i < sections.size(); ++i) {
    const Elf64Shdr& shdr = sections[i];
    if (shdr.type != kElfShtSymtab && shdr.type != kElfShtDynsym) {
      continue;
    }

    if (shdr.entsize != sizeof(Elf64Sym) || shdr.size == 0) {
      continue;
    }

    if (shdr.link >= sections.size() || sections[shdr.link].type != kElfShtStrtab) {
      continue;
    }

    std::vector<uint8_t> strtab;
    if (!read_blob(in, sections[shdr.link].offset, sections[shdr.link].size, &strtab)) {
      continue;
    }
    string_tables[i] = std::move(strtab);

    const size_t sym_count = static_cast<size_t>(shdr.size / shdr.entsize);
    symbol_tables[i].resize(sym_count);

    in.seekg(static_cast<std::streamoff>(shdr.offset), std::ios::beg);
    if (!in) {
      continue;
    }

    for (size_t idx = 0; idx < sym_count; ++idx) {
      if (!read_exact(in, &symbol_tables[i][idx], sizeof(Elf64Sym))) {
        symbol_tables[i].resize(idx);
        break;
      }
    }
  }

  for (size_t i = 0; i < sections.size(); ++i) {
    const Elf64Shdr& shdr = sections[i];
    if (shdr.type != kElfShtSymtab && shdr.type != kElfShtDynsym) {
      continue;
    }

    if (shdr.entsize != sizeof(Elf64Sym) || shdr.size == 0) {
      continue;
    }

    if (i >= string_tables.size()) {
      continue;
    }

    const auto& strtab = string_tables[i];
    const auto& syms = symbol_tables[i];

    for (const auto& sym : syms) {
      const uint8_t type = symbol_type(sym.info);
      if (type == kElfSttNotype && sym.name == 0) {
        continue;
      }

      std::string name = read_string(strtab, sym.name);
      if (name.empty()) {
        continue;
      }

      ghirda::core::Symbol symbol{};
      symbol.name = name;
      symbol.address = sym.value;
      symbol.kind = to_symbol_kind(type);
      program->add_symbol(symbol);

      if (symbol.kind == ghirda::core::SymbolKind::Data && sym.size > 0) {
        ghirda::core::Type type_def{};
        type_def.kind = ghirda::core::TypeKind::Integer;
        type_def.name = name + "_t";
        type_def.size = static_cast<uint32_t>(sym.size);
        program->types().add_type(type_def);
      }
    }
  }

  for (size_t i = 0; i < sections.size(); ++i) {
    const Elf64Shdr& shdr = sections[i];
    if (shdr.type != kElfShtRela && shdr.type != kElfShtRel) {
      continue;
    }

    if (shdr.entsize == 0 || shdr.size == 0) {
      continue;
    }

    if (shdr.link >= sections.size()) {
      continue;
    }

    const auto& symtab = symbol_tables[shdr.link];
    const auto& strtab = string_tables[shdr.link];

    const size_t rel_count = static_cast<size_t>(shdr.size / shdr.entsize);
    in.seekg(static_cast<std::streamoff>(shdr.offset), std::ios::beg);
    if (!in) {
      continue;
    }

    for (size_t idx = 0; idx < rel_count; ++idx) {
      ghirda::core::Relocation relocation{};
      uint32_t type = 0;
      uint32_t sym_index = 0;
      int64_t addend = 0;

      if (shdr.type == kElfShtRela) {
        if (shdr.entsize != sizeof(Elf64Rela)) {
          break;
        }
        Elf64Rela rela{};
        if (!read_exact(in, &rela, sizeof(rela))) {
          break;
        }
        relocation.address = rela.offset;
        type = reloc_type(rela.info);
        sym_index = reloc_sym_index(rela.info);
        addend = rela.addend;
      } else {
        if (shdr.entsize != sizeof(Elf64Rel)) {
          break;
        }
        Elf64Rel rel{};
        if (!read_exact(in, &rel, sizeof(rel))) {
          break;
        }
        relocation.address = rel.offset;
        type = reloc_type(rel.info);
        sym_index = reloc_sym_index(rel.info);
        uint64_t raw_addend = 0;
        if (!program->memory_image().read_u64(rel.offset + program->load_bias(), &raw_addend)) {
          relocation.note = "addend read failed";
        }
        addend = static_cast<int64_t>(raw_addend);
      }

      relocation.type = type;
      relocation.addend = addend;
      if (sym_index < symtab.size()) {
        std::string name = read_string(strtab, symtab[sym_index].name);
        relocation.symbol = name;
      }

      uint64_t symbol_value = 0;
      if (sym_index < symtab.size()) {
        symbol_value = symtab[sym_index].value;
      }

      std::string note;
      bool applied = apply_reloc(type, relocation.address, symbol_value, addend, program->load_bias(),
                                 &program->memory_image(), &note);
      relocation.applied = applied;
      if (!note.empty() && relocation.note.empty()) {
        relocation.note = note;
      }
      if (!applied && relocation.note.empty()) {
        relocation.note = "relocation not applied";
      }
      program->add_relocation(relocation);
    }
  }

  DwarfSections dwarf_sections{};
  std::vector<uint8_t> debug_info;
  std::vector<uint8_t> debug_abbrev;
  std::vector<uint8_t> debug_line;
  std::vector<uint8_t> debug_str;

  for (size_t i = 0; i < sections.size(); ++i) {
    const Elf64Shdr& shdr = sections[i];
    std::string name = read_string(shstrtab, shdr.name);
    if (name == ".debug_info") {
      read_blob(in, shdr.offset, shdr.size, &debug_info);
      dwarf_sections.debug_info.data = &debug_info;
    } else if (name == ".debug_abbrev") {
      read_blob(in, shdr.offset, shdr.size, &debug_abbrev);
      dwarf_sections.debug_abbrev.data = &debug_abbrev;
    } else if (name == ".debug_line") {
      read_blob(in, shdr.offset, shdr.size, &debug_line);
      dwarf_sections.debug_line.data = &debug_line;
    } else if (name == ".debug_str") {
      read_blob(in, shdr.offset, shdr.size, &debug_str);
      dwarf_sections.debug_str.data = &debug_str;
    }
  }

  if (dwarf_sections.debug_info.data && dwarf_sections.debug_abbrev.data) {
    DwarfReader reader(dwarf_sections);
    std::string dwarf_error;
    if (!reader.parse(&program->debug_info(), &dwarf_error)) {
      if (error && error->empty()) {
        *error = "DWARF parse failed: " + dwarf_error;
      }
    }
  }

  if (!program->debug_info().types.empty()) {
    std::unordered_map<uint64_t, const ghirda::core::DebugType*> type_map;
    for (const auto& dt : program->debug_info().types) {
      if (dt.die_offset != 0) {
        type_map[dt.die_offset] = &dt;
      }
    }

    std::unordered_set<uint64_t> emitting;

    std::function<std::pair<std::string, uint32_t>(const ghirda::core::DebugType*)> resolve_type =
        [&](const ghirda::core::DebugType* dt) -> std::pair<std::string, uint32_t> {
      if (!dt) {
        return {"", 0};
      }
      if (dt->die_offset != 0) {
        if (emitting.count(dt->die_offset) != 0) {
          return {dt->name, dt->size};
        }
        emitting.insert(dt->die_offset);
      }

      std::string name = dt->name;
      uint32_t size = dt->size;

      auto resolve_ref = [&](uint64_t ref) -> std::pair<std::string, uint32_t> {
        auto it = type_map.find(ref);
        if (it == type_map.end()) {
          return {"", 0};
        }
        return resolve_type(it->second);
      };

      switch (dt->kind) {
        case ghirda::core::DebugTypeKind::Pointer: {
          auto target = resolve_ref(dt->type_ref);
          std::string base = target.first.empty() ? "void" : target.first;
          name = base + "*";
          if (size == 0) {
            size = 8;
          }
          break;
        }
        case ghirda::core::DebugTypeKind::Const: {
          auto target = resolve_ref(dt->type_ref);
          if (!target.first.empty()) {
            name = "const " + target.first;
          }
          if (size == 0) {
            size = target.second;
          }
          break;
        }
        case ghirda::core::DebugTypeKind::Volatile: {
          auto target = resolve_ref(dt->type_ref);
          if (!target.first.empty()) {
            name = "volatile " + target.first;
          }
          if (size == 0) {
            size = target.second;
          }
          break;
        }
        case ghirda::core::DebugTypeKind::Typedef: {
          auto target = resolve_ref(dt->type_ref);
          if (name.empty() && !target.first.empty()) {
            name = target.first;
          }
          if (size == 0) {
            size = target.second;
          }
          break;
        }
        case ghirda::core::DebugTypeKind::Array: {
          auto target = resolve_ref(dt->type_ref);
          std::string base = target.first.empty() ? "void" : target.first;
          if (dt->array_count != 0) {
            name = base + "[" + std::to_string(dt->array_count) + "]";
          } else {
            name = base + "[]";
          }
          if (size == 0 && target.second != 0 && dt->array_count != 0) {
            size = static_cast<uint32_t>(target.second * dt->array_count);
          }
          break;
        }
        case ghirda::core::DebugTypeKind::Union:
        case ghirda::core::DebugTypeKind::Struct: {
          if (name.empty() && dt->die_offset != 0) {
            name = (dt->kind == ghirda::core::DebugTypeKind::Union ? "union_" : "struct_") +
                   std::to_string(dt->die_offset);
          }
          break;
        }
        case ghirda::core::DebugTypeKind::Enumeration: {
          if (name.empty() && dt->die_offset != 0) {
            name = "enum_" + std::to_string(dt->die_offset);
          }
          break;
        }
        case ghirda::core::DebugTypeKind::Subroutine: {
          name = name.empty() ? "fn" : name;
          if (size == 0) {
            size = 8;
          }
          break;
        }
        default:
          break;
      }

      if (dt->die_offset != 0) {
        emitting.erase(dt->die_offset);
      }

      return {name, size};
    };

    std::unordered_set<uint64_t> emitted;
    for (const auto& dt : program->debug_info().types) {
      if (dt.die_offset == 0 || emitted.count(dt.die_offset) != 0) {
        continue;
      }
      auto resolved = resolve_type(&dt);
      if (resolved.first.empty()) {
        continue;
      }
      ghirda::core::Type type_def{};
      std::string name = resolved.first;
      uint32_t size = resolved.second;

      switch (dt.kind) {
        case ghirda::core::DebugTypeKind::Base:
          type_def.kind = ghirda::core::TypeKind::Integer;
          break;
        case ghirda::core::DebugTypeKind::Pointer: {
          type_def.kind = ghirda::core::TypeKind::Pointer;
          break;
        }
        case ghirda::core::DebugTypeKind::Struct:
        case ghirda::core::DebugTypeKind::Union:
          type_def.kind = ghirda::core::TypeKind::Struct;
          break;
        case ghirda::core::DebugTypeKind::Array:
          type_def.kind = ghirda::core::TypeKind::Array;
          break;
        case ghirda::core::DebugTypeKind::Typedef: {
          type_def.kind = ghirda::core::TypeKind::Integer;
          break;
        }
        case ghirda::core::DebugTypeKind::Const:
        case ghirda::core::DebugTypeKind::Volatile: {
          type_def.kind = ghirda::core::TypeKind::Integer;
          break;
        }
        case ghirda::core::DebugTypeKind::Enumeration:
          type_def.kind = ghirda::core::TypeKind::Integer;
          break;
        case ghirda::core::DebugTypeKind::Subroutine:
          type_def.kind = ghirda::core::TypeKind::Pointer;
          break;
        default:
          type_def.kind = ghirda::core::TypeKind::Void;
          break;
      }

      if (name.empty()) { continue; }
      type_def.name = name;
      type_def.size = size;
      program->types().add_type(type_def);
      emitted.insert(dt.die_offset);
    }
  }

  return true;
}

} // namespace ghirda::loader
