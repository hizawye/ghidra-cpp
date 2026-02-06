#include "ghirda/loader/elf_loader.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "ghirda/core/address_space.h"
#include "ghirda/core/memory_map.h"

namespace ghirda::loader {
namespace {

constexpr std::array<uint8_t, 4> kElfMagic{0x7f, 'E', 'L', 'F'};
constexpr uint16_t kElfClass64 = 2;
constexpr uint16_t kElfDataLittle = 1;
constexpr uint32_t kElfTypeExecutable = 2;
constexpr uint32_t kElfTypeShared = 3;
constexpr uint32_t kElfPtLoad = 1;

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

bool read_exact(std::ifstream& in, void* data, size_t size) {
  in.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(size));
  return in.good();
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

  return true;
}

} // namespace ghirda::loader
