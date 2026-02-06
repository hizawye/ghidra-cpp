#include "ghirda/loader/macho_loader.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "ghirda/core/address_space.h"
#include "ghirda/core/memory_map.h"
#include "ghirda/core/relocation.h"
#include "ghirda/core/symbol.h"

namespace ghirda::loader {
namespace {

#pragma pack(push, 1)
struct MachHeader64 {
  uint32_t magic;
  uint32_t cputype;
  uint32_t cpusubtype;
  uint32_t filetype;
  uint32_t ncmds;
  uint32_t sizeofcmds;
  uint32_t flags;
  uint32_t reserved;
};

struct LoadCommand {
  uint32_t cmd;
  uint32_t cmdsize;
};

struct SegmentCommand64 {
  uint32_t cmd;
  uint32_t cmdsize;
  char segname[16];
  uint64_t vmaddr;
  uint64_t vmsize;
  uint64_t fileoff;
  uint64_t filesize;
  uint32_t maxprot;
  uint32_t initprot;
  uint32_t nsects;
  uint32_t flags;
};

struct Section64 {
  char sectname[16];
  char segname[16];
  uint64_t addr;
  uint64_t size;
  uint32_t offset;
  uint32_t align;
  uint32_t reloff;
  uint32_t nreloc;
  uint32_t flags;
  uint32_t reserved1;
  uint32_t reserved2;
  uint32_t reserved3;
};

struct SymtabCommand {
  uint32_t cmd;
  uint32_t cmdsize;
  uint32_t symoff;
  uint32_t nsyms;
  uint32_t stroff;
  uint32_t strsize;
};

struct Nlist64 {
  uint32_t n_strx;
  uint8_t n_type;
  uint8_t n_sect;
  uint16_t n_desc;
  uint64_t n_value;
};

struct DysymtabCommand {
  uint32_t cmd;
  uint32_t cmdsize;
  uint32_t ilocalsym;
  uint32_t nlocalsym;
  uint32_t iextdefsym;
  uint32_t nextdefsym;
  uint32_t iundefsym;
  uint32_t nundefsym;
  uint32_t tocoff;
  uint32_t ntoc;
  uint32_t modtaboff;
  uint32_t nmodtab;
  uint32_t extrefsymoff;
  uint32_t nextrefsyms;
  uint32_t indirectsymoff;
  uint32_t nindirectsyms;
  uint32_t extreloff;
  uint32_t nextrel;
  uint32_t locreloff;
  uint32_t nlocrel;
};

struct RelocationInfo {
  int32_t r_address;
  uint32_t r_symbolnum : 24,
           r_pcrel : 1,
           r_length : 2,
           r_extern : 1,
           r_type : 4;
};
#pragma pack(pop)

constexpr uint32_t kMachMagic64 = 0xfeedfacf;
constexpr uint32_t kLcSegment64 = 0x19;
constexpr uint32_t kLcSymtab = 0x2;
constexpr uint32_t kLcDysymtab = 0xb;

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

} // namespace

bool MachoLoader::load(const std::string& path, ghirda::core::Program* program, std::string* error) {
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

  MachHeader64 header{};
  if (!read_exact(in, &header, sizeof(header)) || header.magic != kMachMagic64) {
    if (error) {
      *error = "unsupported Mach-O header";
    }
    return false;
  }

  uint64_t min_vaddr = UINT64_MAX;
  uint64_t max_vaddr = 0;

  SymtabCommand symtab{};
  DysymtabCommand dysymtab{};
  bool has_symtab = false;

  std::streamoff cmd_offset = sizeof(MachHeader64);
  for (uint32_t i = 0; i < header.ncmds; ++i) {
    in.seekg(cmd_offset, std::ios::beg);
    LoadCommand lc{};
    if (!read_exact(in, &lc, sizeof(lc)) || lc.cmdsize < sizeof(LoadCommand)) {
      if (error) {
        *error = "failed to read load command";
      }
      return false;
    }

    if (lc.cmd == kLcSegment64 && lc.cmdsize >= sizeof(SegmentCommand64)) {
      SegmentCommand64 seg{};
      in.seekg(cmd_offset, std::ios::beg);
      if (!read_exact(in, &seg, sizeof(seg))) {
        if (error) {
          *error = "failed to read segment";
        }
        return false;
      }

      ghirda::core::Program::Segment ps{};
      ps.vaddr = seg.vmaddr;
      ps.memsz = seg.vmsize;
      ps.filesz = seg.filesize;
      ps.flags = seg.initprot;
      program->add_segment(ps);

      ghirda::core::MemoryRegion region{};
      region.start = seg.vmaddr;
      region.size = seg.vmsize;
      region.readable = (seg.initprot & 1) != 0;
      region.writable = (seg.initprot & 2) != 0;
      region.executable = (seg.initprot & 4) != 0;
      program->memory_map().add_region(region);

      if (seg.filesize != 0) {
        std::vector<uint8_t> bytes;
        if (!read_blob(in, seg.fileoff, seg.filesize, &bytes)) {
          if (error) {
            *error = "failed to read segment bytes";
          }
          return false;
        }
        program->memory_image().map_segment(seg.vmaddr, bytes);
        if (seg.vmsize > seg.filesize) {
          program->memory_image().zero_fill(seg.vmaddr + seg.filesize, seg.vmsize - seg.filesize);
        }
      }

      min_vaddr = std::min(min_vaddr, seg.vmaddr);
      max_vaddr = std::max(max_vaddr, seg.vmaddr + seg.vmsize);

      std::streamoff sect_offset = cmd_offset + sizeof(SegmentCommand64);
      for (uint32_t s = 0; s < seg.nsects; ++s) {
        Section64 sect{};
        in.seekg(sect_offset, std::ios::beg);
        if (!read_exact(in, &sect, sizeof(sect))) {
          if (error) {
            *error = "failed to read section";
          }
          return false;
        }
        ghirda::core::Program::Section sec{};
        sec.name = std::string(sect.sectname, sect.sectname + 16);
        sec.name.erase(std::find(sec.name.begin(), sec.name.end(), '\0'), sec.name.end());
        sec.address = sect.addr;
        sec.size = sect.size;
        sec.file_offset = sect.offset;
        sec.flags = sect.flags;
        if (!sec.name.empty()) {
          program->add_section(sec);
        }
        sect_offset += sizeof(Section64);
      }
    } else if (lc.cmd == kLcSymtab && lc.cmdsize >= sizeof(SymtabCommand)) {
      in.seekg(cmd_offset, std::ios::beg);
      read_exact(in, &symtab, sizeof(symtab));
      has_symtab = true;
    } else if (lc.cmd == kLcDysymtab && lc.cmdsize >= sizeof(DysymtabCommand)) {
      in.seekg(cmd_offset, std::ios::beg);
      read_exact(in, &dysymtab, sizeof(dysymtab));
    }

    cmd_offset += lc.cmdsize;
  }

  if (min_vaddr < max_vaddr) {
    program->add_address_space(ghirda::core::AddressSpace("image", min_vaddr, max_vaddr - min_vaddr));
  }

  if (has_symtab) {
    std::vector<uint8_t> strtab;
    if (read_blob(in, symtab.stroff, symtab.strsize, &strtab)) {
      in.seekg(static_cast<std::streamoff>(symtab.symoff), std::ios::beg);
      for (uint32_t i = 0; i < symtab.nsyms; ++i) {
        Nlist64 sym{};
        if (!read_exact(in, &sym, sizeof(sym))) {
          break;
        }
        std::string name = read_string(strtab, sym.n_strx);
        if (name.empty()) {
          continue;
        }
        ghirda::core::Symbol s{};
        s.name = name;
        s.address = sym.n_value;
        s.kind = ghirda::core::SymbolKind::Function;
        program->add_symbol(s);
      }
    }
  }

  if (dysymtab.nlocrel > 0 && dysymtab.locreloff != 0) {
    in.seekg(static_cast<std::streamoff>(dysymtab.locreloff), std::ios::beg);
    for (uint32_t i = 0; i < dysymtab.nlocrel; ++i) {
      RelocationInfo rel{};
      if (!read_exact(in, &rel, sizeof(rel))) {
        break;
      }
      ghirda::core::Relocation r{};
      r.address = static_cast<uint64_t>(rel.r_address);
      r.type = rel.r_type;
      r.applied = false;
      r.note = "macho reloc";
      program->add_relocation(r);
    }
  }

  return true;
}

} // namespace ghirda::loader
