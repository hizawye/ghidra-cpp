#include "ghirda/loader/elf_loader.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "ghirda/core/address_space.h"
#include "ghirda/core/memory_map.h"
#include "ghirda/core/symbol.h"
#include "ghirda/core/type_system.h"

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
constexpr uint32_t kElfShtDynsym = 11;

constexpr uint8_t kElfSttNotype = 0;
constexpr uint8_t kElfSttObject = 1;
constexpr uint8_t kElfSttFunc = 2;
constexpr uint8_t kElfSttSection = 3;

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

    const size_t sym_count = static_cast<size_t>(shdr.size / shdr.entsize);
    in.seekg(static_cast<std::streamoff>(shdr.offset), std::ios::beg);
    if (!in) {
      continue;
    }

    for (size_t idx = 0; idx < sym_count; ++idx) {
      Elf64Sym sym{};
      if (!read_exact(in, &sym, sizeof(sym))) {
        break;
      }

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

  return true;
}

} // namespace ghirda::loader
